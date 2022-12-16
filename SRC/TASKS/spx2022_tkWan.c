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

typedef enum { WAN_INIT=0, WAN_RECOVER_ID, WAN_CNF_BASE, WAN_CNF_AINPUTS, WAN_CNF_COUNTERS, WAN_DATA } wan_states_t;
static uint8_t wan_state;
static bool wan_state_init(void);
static bool wan_state_recover_id(void);
static bool wan_state_cnf_base(void);
static bool wan_state_cnf_ainputs(void);
static bool wan_state_cnf_counters(void);
static bool wan_state_data(void);

static void wan_process_buffer( uint8_t c);
static void wan_xmit_out(bool debug_flag );

static bool wan_process_recover_rsp(char *p);
static bool wan_process_base_rsp(char *p);
static bool wan_process_ainputs_rsp(char *p);
static bool wan_process_counters_rsp(char *p);
static bool wan_process_data_rsp(char *p);

bool f_process_recover;
bool f_process_base;
bool f_process_ainputs;
bool f_process_counters;
bool f_process_data;

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
    
    wan_state = WAN_INIT;
    
	// loop
	for( ;; )
	{
        switch(wan_state) {
            case WAN_INIT:
                wan_state_init();
                break;
            case WAN_RECOVER_ID:
                wan_state_recover_id();
                break;
            case WAN_CNF_BASE:
                wan_state_cnf_base();
                break;
            case WAN_CNF_AINPUTS:
                wan_state_cnf_ainputs();
                break;
            case WAN_CNF_COUNTERS:
                wan_state_cnf_counters();
                break;
            case WAN_DATA:
                wan_state_data();
                break;
            default:
                xprintf_P(PSTR("ERROR: WAN STATE ??\r\n"));
                wan_state = WAN_INIT;
                break;
        }
        kick_wdt(XWAN_WDG_bp);
        vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
	}    
}
//------------------------------------------------------------------------------
static bool wan_state_init(void)
{
    /*
     * Funciones de inicio
     */
    if (f_debug_comms)
        xprintf_P(PSTR("WAN state INIT.\r\n"));
    
    wan_state = WAN_RECOVER_ID;
    return(true);
}
//------------------------------------------------------------------------------
static bool wan_state_recover_id(void)
{
    /*
     * Funciones de recuperacion del ID del datalogger.
     * Es cuando el ID esta en DEFAULT
     * 
     */
    
uint8_t tryes = 0;
uint8_t fptr;
uint8_t timeout = 0;

    // Si el nombre es diferente de DEFAULT no tengo que hacer nada
    if ( strcmp_P( strupr( systemConf.dlgid), PSTR("DEFAULT")) != 0 ) {
        goto exit_;
    }

    xprintf_P(PSTR("WAN state RECOVER ID.\r\n"));
    f_process_recover = false;
    
    // Armo el buffer
    while ( xSemaphoreTake( sem_WAN, MSTOTAKEWANSEMPH ) != pdTRUE )
        vTaskDelay( ( TickType_t)( 1 ) );
    
    memset(wan_tx_buffer, '\0', WAN_TX_BUFFER_SIZE);
    fptr = 0;
    fptr = sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:RECOVER;UID:%s"), systemConf.dlgid, FW_TYPE, FW_REV, NVM_signature2str());

    // Proceso
    while (tryes++ < 2) {
        // Envio frame
        if ( systemConf.wan_port == WAN_RS485B ) {
            wan_xmit_out(true);
        }
        
        // Espero respuesta
        timeout = 0;
        while ( timeout++ < 10) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            // Esta flag la prende la funcion wan_process_recover_rsp.
            if ( f_process_recover ) {
                goto exit_;
            }
            
        }
        
    }
    
    // Timeout
    xprintf_P(PSTR("ERROR: RECOVER ID:Timeout en server rsp.!!\r\n"));
    
exit_:
                
    xSemaphoreGive( sem_WAN );
        
    wan_state = WAN_CNF_BASE;
    return(true);
}
//------------------------------------------------------------------------------
static bool wan_state_cnf_base(void)
{
    /*
     * Funciones de configuracion de parametros base
     * Enviamos el checksum de:
     * [CNF_BASE:TIMERPOLL]
     * 
     * 
     */
    
uint8_t tryes = 0;
uint8_t hash = 0;
uint8_t fptr;
char *p;
uint8_t timeout = 0;


    xprintf_P(PSTR("WAN state CONFIG BASE.\r\n"));
    f_process_base = false;
    
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
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    
    // Armo el buffer
    while ( xSemaphoreTake( sem_WAN, MSTOTAKEWANSEMPH ) != pdTRUE )
        vTaskDelay( ( TickType_t)( 1 ) );
    
    memset(wan_tx_buffer, '\0', WAN_TX_BUFFER_SIZE);
    fptr = 0;
    fptr = sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:CONF_BASE;UID:%s;HASH:0x%02X"), systemConf.dlgid, FW_TYPE, FW_REV, NVM_signature2str(), hash );

    // Proceso
    while (tryes++ < 2) {
        // Envio frame
        if ( systemConf.wan_port == WAN_RS485B ) {
            wan_xmit_out(true);
        }
        
        // Espero respuesta
        while ( timeout++ < 10) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            if ( f_process_base ) {
                goto exit_;
            }
        }
    }
    
    // Timeout
    xprintf_P(PSTR("ERROR: Timeout en server rsp. CONFIG BASE !!\r\n"));
    
exit_:
                
    xSemaphoreGive( sem_WAN );
        
    wan_state = WAN_CNF_AINPUTS;
    return(true);
}
//------------------------------------------------------------------------------
static bool wan_state_cnf_ainputs(void)
{
    /*
     * Funciones de configuracion de entradas analogicas
     */

uint8_t tryes = 0;
uint8_t i,j;
uint8_t hash = 0;
uint8_t fptr;
char *p;
uint8_t timeout = 0;

    xprintf_P(PSTR("WAN state CONFIG AINPUTS.\r\n"));
    f_process_ainputs = false;

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
    fptr = 0;
    fptr = sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:CONF_AINPUTS;HASH:0x%02X"), systemConf.dlgid, FW_TYPE, FW_REV, hash );

    // Proceso
    while (tryes++ < 2) {
        // Envio frame
        if ( systemConf.wan_port == WAN_RS485B ) {
            wan_xmit_out(true);
        }
        
        // Espero respuesta
        while ( timeout++ < 10) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            if ( f_process_ainputs ) {
                goto exit_;
            }
        }
        
    }
    
    // Timeout
    xprintf_P(PSTR("ERROR: Timeout en server rsp. CONFIG AINPUTS !!\r\n"));
    
exit_:
                
    xSemaphoreGive( sem_WAN ); 

    wan_state = WAN_CNF_COUNTERS;
    
    return(true);
}
//------------------------------------------------------------------------------
static bool wan_state_cnf_counters(void)
{
    /*
     * Funciones de configuracion de contadores
     */

uint8_t tryes = 0;
uint8_t i,j;
uint8_t hash = 0;
uint8_t fptr;
char *p;
uint8_t timeout = 0;

    xprintf_P(PSTR("WAN state CONFIG COUNTERS.\r\n"));
    f_process_counters = false;
    
        // Calculo el hash de la configuracion de las ainputs
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
    fptr = 0;
    fptr = sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:CONF_COUNTERS;HASH:0x%02X"), systemConf.dlgid, FW_TYPE, FW_REV, hash );

    // Proceso
    while (tryes++ < 2) {
        // Envio frame
        if ( systemConf.wan_port == WAN_RS485B ) {
            wan_xmit_out(true);
        }
        
        // Espero respuesta
        while ( timeout++ < 10) {
            vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
            if ( f_process_counters ) {
                goto exit_;
            }
        }
        
    }
    
    // Timeout
    xprintf_P(PSTR("ERROR: Timeout en server rsp. CONFIG COUNTERS !!\r\n"));
    
exit_:
                
    xSemaphoreGive( sem_WAN );  
    wan_state = WAN_DATA;
    return(true);
}
//------------------------------------------------------------------------------
static bool wan_state_data(void)
{
    /*
     * Funciones de transmision de datos
     * No cambio de estado.
     */
    
 static bool entry = true;
    
    if ( entry) {
        entry = false;
        xprintf_P(PSTR("WAN state DATA.\r\n"));
    }
    
    return(true);
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
        xprintf_P(PSTR("Rcvd-> %s\r\n"), p );
        
        if  ( strstr( p, "</html>") != NULL ) {
			// Recibi un frame completo   
            //
            if  ( strstr( p, "CLASS:RECOVER") != NULL ) {
                wan_process_recover_rsp(p);
              
            } else if ( strstr( p, "CLASS:CONF_BASE") != NULL ) {
                wan_process_base_rsp(p);
                
            } else if ( strstr( p, "CLASS:CONF_AINPUTS") != NULL ) {
                wan_process_ainputs_rsp(p);
                
            } else if ( strstr( p, "CLASS:CONF_COUNTERS") != NULL ) {
                wan_process_counters_rsp(p);
                
            } else if ( strstr( p, "CLASS:DATA") != NULL ) {
                wan_process_data_rsp(p);
                
            }  
            // Borramos el buffer para estar listos para el proximo rxframe.
            lBchar_Flush(&wan_lbuffer);
        }
    }
}
//------------------------------------------------------------------------------
static bool wan_process_recover_rsp(char *p)
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
	save_config_in_NVM();
	xprintf_P( PSTR("RECOVER ID: reconfig DLGID to %s\r\n\0"), systemConf.dlgid );
    
    f_process_recover = true;
	return(true);
}
//------------------------------------------------------------------------------
static bool wan_process_base_rsp(char *p)
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

    if  ( strstr( p, "CONFIG:OK") != NULL ) {
       goto exit_;
    }
                
	memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "TPOLL:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// TPOLL
	token = strsep(&stringp,delim);	 	// timerpoll
    config_timerpoll(token);
    xprintf_P( PSTR("BASE_CONF: reconfig TIMERPOLL to %d\r\n\0"), systemConf.timerpoll );
    //
    memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "TDIAL:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// TDIAL
	token = strsep(&stringp,delim);	 	// timerdial
    config_timerdial(token);
    xprintf_P( PSTR("BASE_CONF: reconfig TIMERDIAL to %d\r\n\0"), systemConf.timerdial );
    //
    memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "PWRMODO:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// PWRMODO
	token = strsep(&stringp,delim);	 	// pwrmodo_string
    config_pwrmodo(token);
    xprintf_P( PSTR("BASE_CONF: reconfig PWRMODO to %s\r\n\0"), token );
    //
    memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "PWRON:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// PWRON
	token = strsep(&stringp,delim);	 	// pwron
    config_pwron(token);
    xprintf_P( PSTR("BASE_CONF: reconfig PWRON to %d\r\n\0"), systemConf.pwr_hhmm_on );
    //
    memset(localStr,'\0',sizeof(localStr));
	ts = strstr( p, "PWROFF:");
	strncpy(localStr, ts, sizeof(localStr));
	stringp = localStr;
	token = strsep(&stringp,delim);	 	// PWROFF
	token = strsep(&stringp,delim);	 	// pwroff
    config_pwroff(token);
    xprintf_P( PSTR("BASE_CONF: reconfig PWROFF to %d\r\n\0"), systemConf.pwr_hhmm_off );
    //    
	// Copio el dlgid recibido al systemConf.
	save_config_in_NVM();
    
exit_:
                
    f_process_base = true;
    return(true);    
}
//------------------------------------------------------------------------------
static bool wan_process_ainputs_rsp(char *p)
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
bool save_flag = false;
uint8_t ch;
char str_base[8];

    if  ( strstr( p, "CONFIG:OK") != NULL ) {
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
			xprintf_P( PSTR("AINPUTS_CONF: Reconfig A%d\r\n"), ch);
			save_flag = true;
		}
	}
  
    if (save_flag)
        save_config_in_NVM();

exit_:
                
    f_process_ainputs = true;
    return(true);
}
//------------------------------------------------------------------------------
static bool wan_process_counters_rsp(char *p)
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
bool save_flag = false;
uint8_t ch;
char str_base[8];

    if  ( strstr( p, "CONFIG:OK") != NULL ) {
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
			xprintf_P( PSTR("COUNTERS_CONF: Reconfig C%d\r\n"), ch);
			save_flag = true;
		}
	}

	if ( save_flag ) {
		save_config_in_NVM();
	}

exit_:
                
    f_process_counters = true;
	return(true);

}
//------------------------------------------------------------------------------
static bool wan_process_data_rsp(char *p)
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
            xprintf_P(PSTR("DATA: data_resync_clock ERROR:[%s]\r\n"), token );
            return(false);
        } else {
            data_resync_clock( token, false );
        }
        
    }

    if ( strstr( p, "RESET") != NULL ) {
        xprintf_P(PSTR("DATA: RESET order from Server !!\r\n"));
        vTaskDelay( ( TickType_t)( 2000 / portTICK_PERIOD_MS ) );
        reset();
    }
    return(true);
}
//------------------------------------------------------------------------------
//
// FUNCIONES PUBLICAS
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
bool WAN_xmit_data_frame( dataRcd_s *dataRcd)
{
    // Transmite los datos pasados en la estructura dataRcd por el
    // puerto definido como WAN

uint8_t channel;
bool start;
int16_t fptr;

    /*
     * Si estoy en modo configuracion NO TRANSMITO DATOS !!!
     */

    if (wan_state != WAN_DATA)
        return(false);

    while ( xSemaphoreTake( sem_WAN, MSTOTAKEWANSEMPH ) != pdTRUE )
        vTaskDelay( ( TickType_t)( 1 ) );
    
    // Armo el buffer
    memset(wan_tx_buffer, '\0', WAN_TX_BUFFER_SIZE);
    fptr = 0;
    fptr = sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR("ID:%s;TYPE:%s;VER:%s;CLASS:DATA"), systemConf.dlgid, FW_TYPE, FW_REV);
    
    // Analog Channels:
    for ( channel=0; channel < NRO_ANALOG_CHANNELS; channel++) {
        if ( strcmp ( systemConf.ainputs_conf[channel].name, "X" ) != 0 ) {
            if ( start ) {
                fptr += sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR("%s:%0.2f"), systemConf.ainputs_conf[channel].name, dataRcd->l_ainputs[channel]);
                start = false;
            } else {
                fptr += sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR(";%s:%0.2f"), systemConf.ainputs_conf[channel].name, dataRcd->l_ainputs[channel]);
            }
        }
    }
        
    // Counter Channels:
    for ( channel=0; channel < NRO_COUNTER_CHANNELS; channel++) {
        if ( strcmp ( systemConf.counters_conf[channel].name, "X" ) != 0 ) {
            if ( start ) {
                fptr += sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR("%s:%0.3f"), systemConf.counters_conf[channel].name, dataRcd->l_counters[channel]);
                start = false;
            } else {
                fptr += sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR(";%s:%0.3f"), systemConf.counters_conf[channel].name, dataRcd->l_counters[channel]);
            }
        }
    }
    
    fptr += sprintf_P( (char*)&wan_tx_buffer[fptr], PSTR("\r\n"));
    
    // Transmito
    wan_xmit_out(true);

    xSemaphoreGive( sem_WAN );
    return(true);
        
}
//------------------------------------------------------------------------------
static void wan_xmit_out(bool debug_flag )
{
    /* 
     * Transmite el buffer de tx por el puerto comms configurado para wan.
     */
    
    if ( systemConf.wan_port == WAN_RS485B ) {
        xfprintf_P( fdRS485B, PSTR("%s"), wan_tx_buffer);
    }
    
    if (f_debug_comms || debug_flag) {
        xprintf_P( PSTR("Xmit-> %s\r\n"), &wan_tx_buffer[0]);
    }
    
}
//------------------------------------------------------------------------------
