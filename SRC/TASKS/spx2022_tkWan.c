#include "spx2022.h"
#include "frtos_cmd.h"

SemaphoreHandle_t sem_WAN;
StaticSemaphore_t WAN_xMutexBuffer;
#define MSTOTAKEWANSEMPH ((  TickType_t ) 10 )

#define WAN_TX_BUFFER_SIZE 128
uint8_t wan_tx_buffer[WAN_TX_BUFFER_SIZE];

#define HASH_BUFFER_SIZE 64
uint8_t hash_buffer[HASH_BUFFER_SIZE];

static bool f_debug_comms;

typedef enum { WAN_APAGADO=0, WAN_OFFLINE, WAN_ONLINE_CONFIG, WAN_ONLINE_DATA } wan_states_t;

typedef enum { DATA=0, BLOCK, BLOCKEND } tx_type_t;

static uint8_t wan_state;

struct {
    dataRcd_s dr;
    bool dr_ready;
} drWanBuffer;

static void wan_state_apagado(void);
static void wan_state_offline(void);
static void wan_state_online_config(void);
static void wan_state_online_data(void);
static bool wan_process_frame_linkup(void);
static bool wan_process_frame_recoverId(void);
static bool wan_process_rsp_recoverId(void);
static bool wan_process_frame_configBase(void);
static bool wan_process_rsp_configBase(void);
static bool wan_process_frame_configAinputs(void);
static bool wan_process_rsp_configAinputs(void);
static bool wan_process_frame_configCounters(void);
static bool wan_process_rsp_configCounters(void);
static bool wan_process_buffer_dr(dataRcd_s *dr, tx_type_t tx_type);
static void wan_load_dr_in_txbuffer(dataRcd_s *dr, uint8_t *buff, uint16_t buffer_size, tx_type_t tx_type );
static bool wan_process_rsp_data(void);
static pwr_modo_t wan_check_pwr_modo_now(void);

bool wan_process_from_dump(char *buff, bool ultimo );

#define PRENDER_MODEM() RELE_CLOSE()
#define APAGAR_MODEM() RELE_OPEN()

#define DEBUG_WAN       true
#define NO_DEBUG_WAN    false

bool f_rsp_ping;
bool f_rsp_recoverId;
bool f_rsp_configBase;
bool f_rsp_configAinputs;
bool f_rsp_configCounters;
bool f_rsp_data;
bool f_configurated;

bool f_link_up;

bool f_save_config;

bool f_inicio;

static void wan_process_buffer( uint8_t c);
static void wan_xmit_out(bool debug_flag );
static void wan_print_RXbuffer(void);


//------------------------------------------------------------------------------
void tkWAN(void * pvParameters)
{

	// Esta es la primer tarea que arranca.

( void ) pvParameters;

    while ( ! starting_flag )
        vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

    sem_WAN = xSemaphoreCreateMutexStatic( &WAN_xMutexBuffer );
    lBchar_CreateStatic ( &wan_lbuffer, wan_buffer, WAN_RX_BUFFER_SIZE );
    
    xprintf_P(PSTR("Starting tkWAN..\r\n" ));
    vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );
    
    wan_state = WAN_APAGADO;
    f_inicio = true;
    
	// loop
	for( ;; )
	{
        switch(wan_state) {
            case WAN_APAGADO:
                wan_state_apagado();
                break;
            case WAN_OFFLINE:
                wan_state_offline();
                break;
            case WAN_ONLINE_CONFIG:
                wan_state_online_config();
                break;
            case WAN_ONLINE_DATA:
                wan_state_online_data();
                break;
            default:
                xprintf_P(PSTR("ERROR: WAN STATE ??\r\n"));
                wan_state = WAN_APAGADO;
                break;
        }
        vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
	}    
}
//------------------------------------------------------------------------------
static void wan_state_apagado(void)
{
    /*
     * Implemento la FSM que espera con el modem apagado.
     * Si estoy en modo continuo o modo mixto ( en horario continuo), salgo.
     * En otro caso estoy en modo discreto ( o mixto en horario discreto). En 
     * este caso espero TimerDial.
     */
    
uint16_t sleep_ticks;
pwr_modo_t pwr_modo;

    kick_wdt(XWAN_WDG_bp);
    xprintf_P(PSTR("WAN:: State APAGADO\r\n"));
    
    f_link_up = false;
    
    APAGAR_MODEM();
    vTaskDelay( ( TickType_t)( 5000 / portTICK_PERIOD_MS ) );
    
    // Cuando inicio me conecto siempre sin esperar para configurarme y vaciar la memoria.
    if ( f_inicio ) {
        f_inicio = false;
        goto exit;
    }
    
    pwr_modo = wan_check_pwr_modo_now();
    
    // En modo continuo salgo enseguida. No importa timerdial
    if ( pwr_modo == PWR_CONTINUO) {
        vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
        //xprintf_P(PSTR("WAN:: DEBUG: pwrmodo continuo\r\n"));
        goto exit;
    
    } else {
    
        // Estoy en modo discreto o mixto en horario discreto
        // Espero timerDial
        if ( systemConf.timerdial >= 900 ) {
            // Discreto
            sleep_ticks = systemConf.timerdial;
        } else {
            // Mixto: Me conecto cada 1/2 hora
            sleep_ticks = 1800;
        }
        //xprintf_P(PSTR("WAN:: DEBUG: pwrmodo discreto. Sleep %d secs.\r\n"), sleep_ticks );
        sleep_ticks /= 60;  // paso a minutos
        // Duermo
        while ( sleep_ticks-- > 0) {
            kick_wdt(XWAN_WDG_bp);
            // Espero de a 1 min para poder entrar en tickless.
            vTaskDelay( ( TickType_t)( 60000 / portTICK_PERIOD_MS ) );
        }
    }
    
exit:
    
    PRENDER_MODEM();
    f_configurated = false;    // Fuerzo que se deba configurar
    wan_state = WAN_OFFLINE;
    return;
  
}
//------------------------------------------------------------------------------
static void wan_state_offline(void)
{
    /*
     * Espera que exista link.
     * Para esto envia cada 10s un PING hacia el servidor.
     * Cuando recibe respuesta (PONG), pasa a la fase de enviar
     * los frames de re-configuracion.
     * Las respuestas del server las maneja el callback de respuestas.
     * Este prende flags para indicar al resto que tipo de respuesta llego.
     */
 
    xprintf_P(PSTR("WAN:: State OFFLINE\r\n"));
    kick_wdt(XWAN_WDG_bp);
    
    if ( ! wan_process_frame_linkup() ) {
        wan_state = WAN_APAGADO;
        goto quit;
    }
    
    wan_state = WAN_ONLINE_CONFIG;
    f_link_up = true;   // Si responde al ping asumo que el link esta up.
    
quit:
                
    return;
}
//------------------------------------------------------------------------------
static void wan_state_online_config(void)
{
    /*
     * Se encarga de la configuracion.
     * Solo lo hace una sola vez por lo que tiene una flag estatica que
     * indica si ya se configuro.
     */
  


    xprintf_P(PSTR("WAN:: State ONLINE_CONFIG\r\n"));
    kick_wdt(XWAN_WDG_bp);

    if ( ! wan_process_frame_recoverId() ) {
        wan_state = WAN_APAGADO;
        goto quit;
    }
    
    if ( ! f_configurated ) {
        wan_process_frame_configBase();
        wan_process_frame_configAinputs();
        wan_process_frame_configCounters();
        
        if ( f_save_config ) {
            save_config_in_NVM();
        }
        
        f_configurated = true;
        
        wan_state = WAN_ONLINE_DATA;
        goto quit;
    }
    
    wan_state = WAN_ONLINE_DATA;
    
quit:
                
    return;

}
//------------------------------------------------------------------------------
static void wan_state_online_data(void)
{
    /*
     * Este estado es cuando estamos en condiciones de enviar frames de datos
     * y procesar respuestas.
     * Al entrar, si hay datos en memoria los transmito todos.
     * Luego, si estoy en modo continuo, no hago nada: solo espero.
     * Si estoy en modo discreto, salgo
     */

pwr_modo_t pwr_modo;
uint32_t sleep_time_ms;
uint16_t dumped_rcds = 0;
bool res;

    xprintf_P(PSTR("WAN:: State ONLINE_DATA\r\n"));
    kick_wdt(XWAN_WDG_bp);
    
   
// ENTRY:
    
    // Si hay datos en memoria los transmito todos y los borro en bloques de a 8
    xprintf_P(PSTR("WAN:: ONLINE: dump memory...\r\n"));
    
    while( (dumped_rcds = FS_dump(wan_process_from_dump, 8 )) > 0 ) {
        xprintf_P(PSTR("WAN:: ONLINE dumped %d rcds.\r\n"), dumped_rcds );
        FS_delete(dumped_rcds);
    }
    // 
    pwr_modo = wan_check_pwr_modo_now();
    if ( pwr_modo == PWR_DISCRETO) {
       wan_state = WAN_APAGADO;
       goto quit;
    }
    
    // En modo continuo me quedo esperando por datos para transmitir. 
    while( wan_check_pwr_modo_now() == PWR_CONTINUO ) {
           
        // Hay datos para transmitir
        if ( drWanBuffer.dr_ready ) {
            res =  wan_process_buffer_dr( &drWanBuffer.dr, DATA);
            if (res) {
                drWanBuffer.dr_ready = false;
            } else {
                // Cayo el enlace ???
                f_link_up = false;
                wan_state = WAN_OFFLINE;
                goto quit;
            }
        }
        
        // Hay datos en memoria ?
        
        // Espero que hayan datos
         // Vuelvo a chequear el enlace cada 1 min( tickeless & wdg ).
        kick_wdt(XWAN_WDG_bp);
        sleep_time_ms = ( TickType_t)(  (60000) / portTICK_PERIOD_MS );
        ulTaskNotifyTake( pdTRUE, sleep_time_ms );
        //vTaskDelay( ( TickType_t)( systemConf.timerpoll * 1000 / portTICK_PERIOD_MS ) );      
    }
   
quit:
    
    return;        
}
//------------------------------------------------------------------------------
static pwr_modo_t wan_check_pwr_modo_now(void)
{
    
RtcTimeType_t rtc;
uint16_t now;
uint16_t pwr_on;
uint16_t pwr_off;

   // En modo continuo salgo enseguida. No importa timerdial
    if ( systemConf.pwr_modo == PWR_CONTINUO) {
        //xprintf_P(PSTR("WAN:: DEBUG: CPWM pwrmodo continuo.\r\n"));
        return( PWR_CONTINUO);
    }
    
    // En modo mixto, si estoy en el horario continuo salgo:
    if ( systemConf.pwr_modo == PWR_MIXTO) {
        RTC_read_dtime(&rtc);
        now = rtc.hour * 100 + rtc.min;
		pwr_on = systemConf.pwr_hhmm_on;
		pwr_off = systemConf.pwr_hhmm_off;
        
        // Caso 1:
        if ( pwr_on < pwr_off ) {
            if ( (now > pwr_on) && ( now < pwr_off)) {
                // Estoy en modo MIXTO dentro del horario continuo
                //xprintf_P(PSTR("WAN:: DEBUG: CPWM pwrmodo mixto continuo(A).\r\n"));
                return( PWR_CONTINUO);
            }
        }
        
        // Caso 2:
        if ( pwr_on >= pwr_off ) {
            if ( ( now < pwr_off) || (now > pwr_on)) {
                // Estoy en modo MIXTO dentro del horario continuo
                //xprintf_P(PSTR("WAN:: CPWM DEBUG: pwrmodo mixto continuo(B).\r\n"));
                return( PWR_CONTINUO);
            }                
        }
    }
    
    // Estoy en modo discreto o mixto en horario discreto
    //xprintf_P(PSTR("WAN:: DEBUG: CPWM pwrmodo discreto.\r\n"));
    return( PWR_DISCRETO);
}
//------------------------------------------------------------------------------
static bool wan_process_frame_linkup(void)
{
    /*
     * Envia los PING para determinar si el enlace está establecido
     * Intento durante 2 minutos mandando un ping cada 10s.
     */
    
uint16_t tryes;
uint16_t timeout;
bool retS = false;

    xprintf_P(PSTR("WAN:: LINK.\r\n"));
 
    // Armo el frame
    while ( xSemaphoreTake( sem_WAN, MSTOTAKEWANSEMPH ) != pdTRUE )
        vTaskDelay( ( TickType_t)( 1 ) );
    memset(wan_tx_buffer, '\0', WAN_TX_BUFFER_SIZE);
    sprintf_P( (char*)&wan_tx_buffer, PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:PING"), systemConf.dlgid, FW_TYPE, FW_REV );
        
    // Proceso
    f_rsp_ping = false;
    tryes = 12;
    while (tryes-- > 0) {
        
        kick_wdt(XWAN_WDG_bp);   
        wan_xmit_out(DEBUG_WAN);
    
        // Espero respuesta chequeando cada 1s durante 10s.
        timeout = 15;
        while ( timeout-- > 0) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            if ( f_rsp_ping ) {
                wan_print_RXbuffer();
                xprintf_P(PSTR("WAN:: Link up.\r\n"));
                retS = true;
                goto exit_;
            }
        }
    }
    
    // Expiro el tiempo sin respuesta del server.
    xprintf_P(PSTR("WAN:: Link up timeout !!\r\n"));
    retS = false;
    
exit_:
               
    xSemaphoreGive( sem_WAN );
    return(retS);
 
}
//------------------------------------------------------------------------------
static bool wan_process_frame_recoverId(void)
{
    /*
     * Este proceso es para recuperar el ID cuando localmente está en DEFAULT.
     * Intento 2 veces mandar el frame.
     */
    
uint8_t tryes = 0;
uint8_t timeout = 0;
uint8_t fptr;
bool retS = false;

    xprintf_P(PSTR("WAN:: RECOVERID.\r\n"));

    // Si el nombre es diferente de DEFAULT no tengo que hacer nada
    if ( strcmp_P( strupr( systemConf.dlgid), PSTR("DEFAULT")) != 0 ) {
        return(true);
    }

    // Armo el buffer
    while ( xSemaphoreTake( sem_WAN, MSTOTAKEWANSEMPH ) != pdTRUE )
        vTaskDelay( ( TickType_t)( 1 ) );
    memset(wan_tx_buffer, '\0', WAN_TX_BUFFER_SIZE);
    fptr = 0;
    fptr = sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:RECOVER;UID:%s"), systemConf.dlgid, FW_TYPE, FW_REV, NVM_signature2str());

    // Proceso. Envio hasta 2 veces el frame y espero hasta 10s la respuesta
    f_rsp_recoverId = false;
    tryes = 2;
    while (tryes-- > 0) {
        
        kick_wdt(XWAN_WDG_bp);     
        wan_xmit_out(DEBUG_WAN);
    
        // Espero respuesta chequeando cada 1s durante 10s.
        timeout = 10;
        while ( timeout-- > 0) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            if ( f_rsp_recoverId ) {
                wan_print_RXbuffer();
                wan_process_rsp_recoverId();
                retS = true;
                goto exit_;
            }
        }
    }
 
    // Expiro el tiempo sin respuesta del server.
    xprintf_P(PSTR("WAN:: RECOVERID ERROR:Timeout en server rsp.!!\r\n"));
    retS = false;
    
exit_:
               
    xSemaphoreGive( sem_WAN );

    return(retS);
}
//------------------------------------------------------------------------------
static bool wan_process_rsp_recoverId(void)
{
    /*
     * Extraemos el DLGID del frame y lo reconfiguramos
     * RXFRAME: <html><body><h1>CLASS:RECOVER;DLGID:xxxx</h1></body></html>
     *          <html><body><h1>CLASS:RECOVER;DLGID:DEFAULT</h1></body></html>
     */
    
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",;:=><";
char *ts = NULL;
char *p;

    p = lBchar_get_buffer(&wan_lbuffer);
	memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "RECOVER;");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	    // RECOVER
	token = strsep(&stringp,delim);	 	// DLGID
	token = strsep(&stringp,delim);	 	// TEST01
	// Copio el dlgid recibido al systemConf.
	memset(systemConf.dlgid,'\0', sizeof(systemConf.dlgid) );
	strncpy( systemConf.dlgid, token, DLGID_LENGTH);
	
    f_save_config = true;
    //save_config_in_NVM();
	xprintf_P( PSTR("WAN:: Reconfig DLGID to %s\r\n\0"), systemConf.dlgid );
	return(true);
}
//------------------------------------------------------------------------------
static bool wan_process_frame_configBase(void)
{
     /*
      * Envo un frame con el hash de la configuracion BASE.
      * El server me puede mandar OK o la nueva configuracion que debo tomar.
      * Lo reintento 2 veces
      */
    
uint8_t tryes = 0;
uint8_t timeout = 0;
bool retS = false;
uint8_t hash = 0;
char *p;

    xprintf_P(PSTR("WAN:: CONFIG_BASE.\r\n"));

    // Calculo el hash de la configuracion base
    memset(hash_buffer, '\0', HASH_BUFFER_SIZE);
    sprintf_P( (char *)&hash_buffer, PSTR("[TIMERPOLL:%03d]"), systemConf.timerpoll );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    //
    memset(hash_buffer, '\0', HASH_BUFFER_SIZE);
    sprintf_P( (char *)&hash_buffer, PSTR("[TIMERDIAL:%03d]"), systemConf.timerdial );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );    
    //
    memset(hash_buffer, '\0', HASH_BUFFER_SIZE);
    sprintf_P( (char *)&hash_buffer, PSTR("[PWRMODO:%d]"), systemConf.pwr_modo );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    //
    memset(hash_buffer, '\0', HASH_BUFFER_SIZE);
    sprintf_P( (char *)&hash_buffer, PSTR("[PWRON:%04d]"), systemConf.pwr_hhmm_on );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    //
    memset(hash_buffer, '\0', HASH_BUFFER_SIZE);
    sprintf_P( (char *)&hash_buffer, PSTR("[PWROFF:%04d]"), systemConf.pwr_hhmm_off );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    
    // Armo el buffer
    while ( xSemaphoreTake( sem_WAN, MSTOTAKEWANSEMPH ) != pdTRUE )
        vTaskDelay( ( TickType_t)( 1 ) );
    memset(wan_tx_buffer, '\0', WAN_TX_BUFFER_SIZE);
    sprintf_P( (char*)&wan_tx_buffer, PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:CONF_BASE;UID:%s;HASH:0x%02X"), systemConf.dlgid, FW_TYPE, FW_REV, NVM_signature2str(), hash );

    // Proceso. Envio hasta 2 veces el frame y espero hasta 10s la respuesta
    f_rsp_configBase = false;
    tryes = 2;
    while (tryes-- > 0) {
        
        kick_wdt(XWAN_WDG_bp);     
        wan_xmit_out(DEBUG_WAN);
    
        // Espero respuesta chequeando cada 1s durante 10s.
        timeout = 10;
        while ( timeout-- > 0) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            if ( f_rsp_configBase ) {
                wan_print_RXbuffer();
                wan_process_rsp_configBase();
                retS = true;
                goto exit_;
            }
        }
    }
 
    // Expiro el tiempo sin respuesta del server.
    xprintf_P(PSTR("WAN:: CONFIG_BASE ERROR: Timeout en server rsp.!!\r\n"));
    retS = false;
    
exit_:
               
    xSemaphoreGive( sem_WAN );
    return(retS);   
}
//------------------------------------------------------------------------------
static bool wan_process_rsp_configBase(void)
{
    /*
     * Envia el TPOLL
     * RXFRAME: <html><body><h1>CLASS:CONF_BASE;TPOLL:60;TDIAL:0;PWRMODO:MIXTO;PWRON:2330;PWROFF:0630</h1></body></html>                        
     *                          CLASS:CONF_BASE;CONFIG:OK
     * 
     */
    
char localStr[32] = { 0 };
char *stringp = NULL;
char *token = NULL;
char *delim = ",;:=><";
char *ts = NULL;
char *p;
bool retS = false;

    p = lBchar_get_buffer(&wan_lbuffer);
    
    if  ( strstr( p, "CONFIG:OK") != NULL ) {
        retS = true;
       goto exit_;
    }
                
	memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "TPOLL:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// TPOLL
	token = strsep(&stringp,delim);	 	// timerpoll
    config_timerpoll(token);
    xprintf_P( PSTR("WAN:: Reconfig TIMERPOLL to %d\r\n\0"), systemConf.timerpoll );
    //
    memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "TDIAL:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// TDIAL
	token = strsep(&stringp,delim);	 	// timerdial
    config_timerdial(token);
    xprintf_P( PSTR("WAN:: Reconfig TIMERDIAL to %d\r\n\0"), systemConf.timerdial );
    //
    memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "PWRMODO:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// PWRMODO
	token = strsep(&stringp,delim);	 	// pwrmodo_string
    config_pwrmodo(token);
    xprintf_P( PSTR("WAN:: Reconfig PWRMODO to %s\r\n\0"), token );
    //
    memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "PWRON:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// PWRON
	token = strsep(&stringp,delim);	 	// pwron
    config_pwron(token);
    xprintf_P( PSTR("WAN:: Reconfig PWRON to %d\r\n\0"), systemConf.pwr_hhmm_on );
    //
    memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "PWROFF:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// PWROFF
	token = strsep(&stringp,delim);	 	// pwroff
    config_pwroff(token);
    xprintf_P( PSTR("WAN:: Reconfig PWROFF to %d\r\n\0"), systemConf.pwr_hhmm_off );
    //    
	// Copio el dlgid recibido al systemConf.
    f_save_config = true;
	//save_config_in_NVM();
    retS = true;
    
exit_:
                
    return(retS);      
}
//------------------------------------------------------------------------------
static bool wan_process_frame_configAinputs(void)
{
     /*
      * Envo un frame con el hash de la configuracion de entradas analogicas.
      * El server me puede mandar OK o la nueva configuracion que debo tomar.
      * Lo reintento 2 veces
      */
    
uint8_t tryes = 0;
uint8_t timeout = 0;
uint8_t i,j;
bool retS = false;
uint8_t hash = 0;
char *p;

    xprintf_P(PSTR("WAN:: CONFIG_AINPUTS.\r\n"));

   // Calculo el hash de la configuracion de las ainputs
    for(i=0; i<NRO_ANALOG_CHANNELS; i++) {
        memset(hash_buffer, '\0', HASH_BUFFER_SIZE);
        j = 0;
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("[A%d:%s,"), i, systemConf.ainputs_conf[i].name );
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("%d,%d,"), systemConf.ainputs_conf[i].imin, systemConf.ainputs_conf[i].imax );
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("%.02f,%.02f,"), systemConf.ainputs_conf[i].mmin, systemConf.ainputs_conf[i].mmax );
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("%.02f]"), systemConf.ainputs_conf[i].offset);    
        p = (char *)hash_buffer;
        while (*p != '\0') {
            hash = u_hash(hash, *p++);
        }
        //xprintf_P(PSTR("HASH_AIN:<%s>, hash=%d\r\n"),hash_buffer, hash );
    }
 
    // Armo el buffer
    while ( xSemaphoreTake( sem_WAN, MSTOTAKEWANSEMPH ) != pdTRUE )
        vTaskDelay( ( TickType_t)( 1 ) );
    memset(wan_tx_buffer, '\0', WAN_TX_BUFFER_SIZE);
    sprintf_P( (char*)&wan_tx_buffer, PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:CONF_AINPUTS;HASH:0x%02X"), systemConf.dlgid, FW_TYPE, FW_REV, hash );

    // Proceso. Envio hasta 2 veces el frame y espero hasta 10s la respuesta
    f_rsp_configAinputs = false;
    tryes = 2;
    while (tryes-- > 0) {
        
        kick_wdt(XWAN_WDG_bp);     
        wan_xmit_out(DEBUG_WAN);
    
        // Espero respuesta chequeando cada 1s durante 10s.
        timeout = 10;
        while ( timeout-- > 0) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            if ( f_rsp_configAinputs ) {
                wan_print_RXbuffer();
                wan_process_rsp_configAinputs();
                retS = true;
                goto exit_;
            }
        }
    }
 
    // Expiro el tiempo sin respuesta del server.
    xprintf_P(PSTR("WAN:: CONFIG_AINPUTS ERROR: Timeout en server rsp.!!\r\n"));
    retS = false;
    
exit_:
               
    xSemaphoreGive( sem_WAN );
    return(retS);      
}
//------------------------------------------------------------------------------
static bool wan_process_rsp_configAinputs(void)
{
   /*
     * Procesa la configuracion de los canales analogicos
     * RXFRAME: <html><body><h1>CLASS:CONF_AINPUTS;A0:pA,4,20,0.0,10.0,0.0;A1:pB,4,20,0.0,10.0,0.0;A2:X,4,20,0.0,10.0,0.0;</h1></body></html>
     *                          CLASS:CONF_AINPUTS;CONFIG:OK
     */
    
char *ts = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_name= NULL;
char *tk_iMin= NULL;
char *tk_iMax = NULL;
char *tk_mMin = NULL;
char *tk_mMax = NULL;
char *tk_offset = NULL;
char *delim = ",;:=><";
uint8_t ch;
char str_base[8];
char *p;
bool retS = false;

    p = lBchar_get_buffer(&wan_lbuffer);
    
    if  ( strstr( p, "CONFIG:OK") != NULL ) {
        retS = true;
       goto exit_;
    }

	// A?
	for (ch=0; ch < NRO_ANALOG_CHANNELS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("A%d\0"), ch );

		if ( strstr( p, str_base) != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
            ts = strstr( p, str_base);
			strncpy( localStr, ts, sizeof(localStr));
			stringp = localStr;
			tk_name = strsep(&stringp,delim);		//A0
			tk_name = strsep(&stringp,delim);		//name
			tk_iMin = strsep(&stringp,delim);		//iMin
			tk_iMax = strsep(&stringp,delim);		//iMax
			tk_mMin = strsep(&stringp,delim);		//mMin
			tk_mMax = strsep(&stringp,delim);		//mMax
			tk_offset = strsep(&stringp,delim);		//offset

			ainputs_config_channel( ch, systemConf.ainputs_conf, tk_name ,tk_iMin, tk_iMax, tk_mMin, tk_mMax, tk_offset );
			xprintf_P( PSTR("WAN:: Reconfig A%d\r\n"), ch);
			f_save_config = true;
		}
	}
    retS = true;
   
exit_:
                
    return(retS);
}
//------------------------------------------------------------------------------
static bool wan_process_frame_configCounters(void)
{
     /*
      * Envo un frame con el hash de la configuracion de contadores.
      * El server me puede mandar OK o la nueva configuracion que debo tomar.
      * Lo reintento 2 veces
      */
    
uint8_t tryes = 0;
uint8_t timeout = 0;
uint8_t i,j;
bool retS = false;
uint8_t hash = 0;
char *p;

    xprintf_P(PSTR("WAN:: CONFIG_COUNTERS.\r\n"));
    
    // Calculo el hash de la configuracion de los contadores
    for(i=0; i < NRO_COUNTER_CHANNELS; i++) {

        memset(hash_buffer, '\0', HASH_BUFFER_SIZE);
        j = 0;
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("[C%d:%s,"), i, systemConf.counters_conf[i].name );
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("%.03f,%d]"), systemConf.counters_conf[i].magpp, systemConf.counters_conf[i].modo_medida );
            
        p = (char *)hash_buffer;
        while (*p != '\0') {
            hash = u_hash(hash, *p++);
        }
        //xprintf_P(PSTR("HASH_CNT:<%s>, hash=%d\r\n"),hash_buffer, hash );
    }
    
    // Armo el buffer
    while ( xSemaphoreTake( sem_WAN, MSTOTAKEWANSEMPH ) != pdTRUE )
        vTaskDelay( ( TickType_t)( 1 ) );
    
    memset(wan_tx_buffer, '\0', WAN_TX_BUFFER_SIZE);
    sprintf_P( (char*)&wan_tx_buffer, PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:CONF_COUNTERS;HASH:0x%02X"), systemConf.dlgid, FW_TYPE, FW_REV, hash );

    // Proceso. Envio hasta 2 veces el frame y espero hasta 10s la respuesta
    f_rsp_configCounters = false;
    tryes = 2;
    while (tryes-- > 0) {
        
        kick_wdt(XWAN_WDG_bp);     
        wan_xmit_out(DEBUG_WAN);
    
        // Espero respuesta chequeando cada 1s durante 10s.
        timeout = 10;
        while ( timeout-- > 0) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            if ( f_rsp_configCounters ) {
                wan_print_RXbuffer();
                wan_process_rsp_configCounters();
                retS = true;
                goto exit_;
            }
        }
    }
 
    // Expiro el tiempo sin respuesta del server.
    xprintf_P(PSTR("WAN:: CONFIG_COUNTERS ERROR: Timeout en server rsp.!!\r\n"));
    retS = false;
    
exit_:
               
    xSemaphoreGive( sem_WAN );
    return(retS);   
    
 
}
//------------------------------------------------------------------------------
static bool wan_process_rsp_configCounters(void)
{
    /*
     * Procesa la configuracion de los canales contadores
     * RXFRAME: <html><body><h1>CLASS:CONF_COUNTERS;C0:q0,0.01,CAUDAL;C1:X,0.0,CAUDAL;</h1></body></html>
     * 
     */

char *ts = NULL;
char localStr[32] = { 0 };
char *stringp = NULL;
char *tk_name = NULL;
char *tk_magpp = NULL;
char *tk_modo = NULL;
char *delim = ",;:=><";
uint8_t ch;
char str_base[8];
char *p;
bool retS = false;

    p = lBchar_get_buffer(&wan_lbuffer);
    
    if  ( strstr( p, "CONFIG:OK") != NULL ) {
       retS = true;
       goto exit_;
    }

	// C?
	for (ch=0; ch < NRO_COUNTER_CHANNELS; ch++ ) {
		memset( &str_base, '\0', sizeof(str_base) );
		snprintf_P( str_base, sizeof(str_base), PSTR("C%d"), ch );

		if ( strstr( p, str_base) != NULL ) {
			memset(localStr,'\0',sizeof(localStr));
			ts = strstr( p, str_base);
			strncpy(localStr, ts, sizeof(localStr));
			stringp = localStr;
			tk_name = strsep(&stringp,delim);		//C0
			tk_name = strsep(&stringp,delim);		//name
			tk_magpp = strsep(&stringp,delim);		//magpp
			tk_modo = strsep(&stringp,delim);

			counters_config_channel( ch ,systemConf.counters_conf, tk_name , tk_magpp, tk_modo );
			xprintf_P( PSTR("WAN:: Reconfig C%d\r\n"), ch);
			f_save_config = true;
		}
	}
    
    retS = true;
    
exit_:
               
	return(retS);

}
//------------------------------------------------------------------------------
static bool wan_process_buffer_dr(dataRcd_s *dr, tx_type_t tx_type)
{
    
   /*
    * Armo el wan_tx_buffer.
    * Transmite los datos pasados en la estructura dataRcd por el
    * puerto definido como WAN
    */ 

uint8_t tryes = 0;
uint8_t timeout = 0;
bool retS = false;

    xprintf_P(PSTR("WAN:: DATA.\r\n"));

    while ( xSemaphoreTake( sem_WAN, MSTOTAKEWANSEMPH ) != pdTRUE )
        vTaskDelay( ( TickType_t)( 1 ) );
    
    // Formateo(escribo) el dr en el wan_tx_buffer
    wan_load_dr_in_txbuffer(dr, (uint8_t *)&wan_tx_buffer,WAN_TX_BUFFER_SIZE, tx_type);
    
    // Proceso. Envio hasta 2 veces el frame y espero hasta 10s la respuesta
    f_rsp_data = false;
    tryes = 2;
    while (tryes-- > 0) {
        
        kick_wdt(XWAN_WDG_bp);     
        wan_xmit_out(DEBUG_WAN);
    
        // En los casos de BLOCK, no espero respuesta.
        /*
        if ( tx_type == BLOCK ) {
            vTaskDelay( ( TickType_t)( 250 / portTICK_PERIOD_MS ) );
            retS = true;
            goto exit_;
        }
        */
        
        // Espero respuesta chequeando cada 1s durante 10s.
        timeout = 10;
        while ( timeout-- > 0) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            if ( f_rsp_data ) {
                wan_print_RXbuffer();
                wan_process_rsp_data();
                retS = true;
                goto exit_;
            }
        }
        
        xprintf_P(PSTR("WAN:: DATA RETRY\r\n"));
        
    }
 
    // Expiro el tiempo sin respuesta del server.
    xprintf_P(PSTR("WAN:: DATA ERROR: Timeout en server rsp.!!\r\n"));
    retS = false;
    
exit_:

    xSemaphoreGive( sem_WAN );
    return(retS);
    
}
//------------------------------------------------------------------------------
static void wan_load_dr_in_txbuffer(dataRcd_s *dr, uint8_t *buff, uint16_t buffer_size, tx_type_t tx_type )
{
    /*
     * Toma un puntero a un dr y el wan_tx_buffer y escribe el dr en este
     * con el formato para transmitir.
     * En semaforo lo debo tomar en la funcion que invoca !!!.
     */

uint8_t channel;
int16_t fptr;

   // Armo el buffer
    memset(buff, '\0', buffer_size);
    fptr = 0;
    fptr = sprintf_P( (char *)&buff[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:DATA;"), systemConf.dlgid, FW_TYPE, FW_REV);   
    
    /*
    switch(tx_type) {
        case DATA:
            fptr = sprintf_P( (char *)&buff[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:DATA;"), systemConf.dlgid, FW_TYPE, FW_REV);        
            break;
        case BLOCK:
            fptr = sprintf_P( (char *)&buff[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:BLOCK;"), systemConf.dlgid, FW_TYPE, FW_REV);        
            break;
        case BLOCKEND:
            fptr = sprintf_P( (char *)&buff[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:BLOCKEND;"), systemConf.dlgid, FW_TYPE, FW_REV);        
            break;
        default:
            return;          
    }
    */
    
    
    // Clock
    fptr += sprintf_P( (char *)&buff[fptr], PSTR("DATE:%02d%02d%02d;"), dr->rtc.year,dr->rtc.month, dr->rtc.day );
    fptr += sprintf_P( (char *)&buff[fptr], PSTR("TIME:%02d%02d%02d;"), dr->rtc.hour,dr->rtc.min, dr->rtc.sec);
    
    // Analog Channels:
    for ( channel=0; channel < NRO_ANALOG_CHANNELS; channel++) {
        if ( strcmp ( systemConf.ainputs_conf[channel].name, "X" ) != 0 ) {
            fptr += sprintf_P( (char*)&buff[fptr], PSTR("%s:%0.2f;"), systemConf.ainputs_conf[channel].name, dr->l_ainputs[channel]);
        }
    }
        
    // Counter Channels:
    for ( channel=0; channel < NRO_COUNTER_CHANNELS; channel++) {
        if ( strcmp ( systemConf.counters_conf[channel].name, "X" ) != 0 ) {
            fptr += sprintf_P( (char*)&buff[fptr], PSTR("%s:%0.3f;"), systemConf.counters_conf[channel].name, dr->l_counters[channel]);
        }
    }
    
    fptr += sprintf_P( (char*)&buff[fptr], PSTR("\r\n"));
    
}
//------------------------------------------------------------------------------
static bool wan_process_rsp_data(void)
{
   /*
     * Procesa las respuestas a los frames DATA
     * Podemos recibir CLOCK, RESET
     */
    
char localStr[32] = { 0 };
char *ts = NULL;
char *stringp = NULL;
char *token = NULL;
char *delim = ",;:=><";
char *p;

    p = lBchar_get_buffer(&wan_lbuffer);

    if ( strstr( p, "CLOCK") != NULL ) {
        memset(localStr,'\0',sizeof(localStr));
        ts = strstr( p, "CLOCK:");
        strncpy(localStr, ts, sizeof(localStr));
        stringp = localStr;
        token = strsep(&stringp,delim);			// CLOCK
        token = strsep(&stringp,delim);			// 1910120345

        // Error en el string recibido
        if ( strlen(token) < 10 ) {
            // Hay un error en el string que tiene la fecha.
            // No lo reconfiguro
            xprintf_P(PSTR("WAN:: data_resync_clock ERROR:[%s]\r\n"), token );
            return(false);
        } else {
            data_resync_clock( token, false );
        }
        
    }

    if ( strstr( p, "RESET") != NULL ) {
        xprintf_P(PSTR("WAN:: RESET order from Server !!\r\n"));
        vTaskDelay( ( TickType_t)( 2000 / portTICK_PERIOD_MS ) );
        reset();
    }
    
    f_rsp_data = false;
    return(true);  
}
//------------------------------------------------------------------------------
static void wan_xmit_out(bool debug_flag )
{
    /* 
     * Transmite el buffer de tx por el puerto comms configurado para wan.
     */
    
    // Antes de trasmitir siempre borramos el Rxbuffer
    lBchar_Flush(&wan_lbuffer);
        
    if ( systemConf.wan_port == WAN_RS485B ) {
        xfprintf_P( fdRS485B, PSTR("%s"), &wan_tx_buffer[0]);
    }
    
    if (f_debug_comms || debug_flag) {
        xprintf_P( PSTR("Xmit-> %s\r\n"), &wan_tx_buffer[0]);
    }
    
}
//------------------------------------------------------------------------------
static void wan_process_buffer( uint8_t c)
{
    /*
     * Funcion invocada cada caracter que se ingresa a la cola wan
     * para su procesamiento
     * Cuando termina un frame (\r\n) y es correcto (</html>) invocamos
     * una funcion mas especializada
     */
    
char *p;
    
    if (( c == '\n') || ( c == '\r')) {
        
        p = lBchar_get_buffer(&wan_lbuffer);
        
        if  ( strstr( p, "</html>") != NULL ) {
			// Recibi un frame completo   
            //
            if ( strstr( p, "CLASS:PONG") != NULL ) {
                f_rsp_ping = true;
                
            } else if  ( strstr( p, "CLASS:RECOVER") != NULL ) {
                f_rsp_recoverId = true;
              
            } else if ( strstr( p, "CLASS:CONF_BASE") != NULL ) {
                f_rsp_configBase = true;
                
            } else if ( strstr( p, "CLASS:CONF_AINPUTS") != NULL ) {
                f_rsp_configAinputs = true;
                
            } else if ( strstr( p, "CLASS:CONF_COUNTERS") != NULL ) {
                f_rsp_configCounters = true;
                
            } else if ( strstr( p, "CLASS:DATA") != NULL ) {
                f_rsp_data = true;
                
            }  
        }
    }   
}
//------------------------------------------------------------------------------
static void wan_print_RXbuffer(void)
{

char *p;
            
    p = lBchar_get_buffer(&wan_lbuffer);
    xprintf_P(PSTR("Rcvd-> %s\r\n"), p );
}
//------------------------------------------------------------------------------
bool wan_process_from_dump(char *buff, bool ultimo )
{
    /*
     * Funcion pasada a FS_dump() para que formatee el buffer y transmita
     * como un dr.
     * El parametro ultimo es para indicar que el frame es el ultimo del bloque
     * por lo tanto debemos poner el tag BLOCKEND para que el server mande el 
     * ACK
     * 
     */
    
dataRcd_s dr;
bool retS = false;

    kick_wdt(XWAN_WDG_bp);
    memcpy ( &dr, buff, sizeof(dataRcd_s) );
    if ( ultimo ) {
        retS = wan_process_buffer_dr(&dr, BLOCKEND );
    } else {
        retS = wan_process_buffer_dr(&dr, BLOCK );
    }
    return(retS);
}
//------------------------------------------------------------------------------
//
// FUNCIONES PUBLICAS
//
//------------------------------------------------------------------------------
bool WAN_process_data_rcd( dataRcd_s *dataRcd)
{
    /*
     * El procesamiento implica transmitir si estoy con el link up, o almacenarlo
     * en memoria para luego transmitirlo.
     * Copio el registro a uno local y prendo la flag que indica que hay datos 
     * para transmitir.
     * Si hay un dato aun en el buffer, lo sobreescribo !!!
     * 
     */
   
bool retS = false;
fat_s l_fat;   

    if ( f_link_up ) {
        /*
         * Hay enlace. 
         * Guardo el dato en el buffer.
         * Indico que hay un dato listo para enviar
         * Aviso (despierto) para que se transmita.
         */
        memcpy( &drWanBuffer.dr, dataRcd, sizeof(dataRcd_s));
        drWanBuffer.dr_ready = true;    
        // Aviso al estado online que hay un frame listo
        while ( xTaskNotify(xHandle_tkWAN, SGN_FRAME_READY , eSetBits ) != pdPASS ) {
			vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
		}
        retS = true;
        
    } else {
        // Guardo en memoria
        xprintf_P(PSTR("WAN: Save frame in EE.\r\n"));
        retS = FS_writeRcd( dataRcd, sizeof(dataRcd_s) );
        if ( ! retS  ) {
            // Error de escritura o memoria llena ??
            xprintf_P(PSTR("WAN:: WR ERROR\r\n"));
        }
        // Stats de memoria
        FAT_read(&l_fat);
        xprintf_P( PSTR("wrPtr=%d,rdPtr=%d,count=%d\r\n"),l_fat.head,l_fat.tail, l_fat.count );
        
    }
    return(retS);
}
//------------------------------------------------------------------------------
void WAN_put(uint8_t c)
{
    /*
     * Funcion usada por rutinas que manejan los RX de los diferentes comm ports
     * para poner los datos recividos en la cola WAN cuando estos puertos se
     * configuran para WAN
     */
    
    lBchar_Put( &wan_lbuffer, c);
    wan_process_buffer(c);
}
//------------------------------------------------------------------------------
void WAN_print_configuration(void)
{
    
    xprintf_P(PSTR("Comms:\r\n"));
    xprintf_P(PSTR(" debug: "));
    f_debug_comms ? xprintf_P(PSTR("true\r\n")) : xprintf_P(PSTR("false\r\n"));
    
    switch(systemConf.wan_port) {
    case WAN_RS485B:
        xprintf_P(PSTR(" port: RS485B\r\n"));
        break;
    case WAN_NBIOT:
        xprintf_P(PSTR(" port: NbIOT\r\n"));
        break;
    default:
        xprintf_P(PSTR(" type: ERROR\r\n"));
        break;
    } 
    
}
//------------------------------------------------------------------------------
void WAN_config_debug(bool debug )
{
    if ( debug ) {
        f_debug_comms = true;
    } else {
        f_debug_comms = false;
    }
}
//------------------------------------------------------------------------------
bool WAN_read_debug(void)
{
    return (f_debug_comms);
}
//------------------------------------------------------------------------------
