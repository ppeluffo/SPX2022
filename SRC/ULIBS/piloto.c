
#include "piloto.h"
#include "ringBuffer.h"
//#include "spx2022.h"
#include <stdio.h>

// Configurar ch_pA, ch_pB x cmd y por WAN

typedef struct {
	float presion;
} s_pltRb_t;

#define PILOTO_STORAGE_SIZE 5

s_pltRb_t pRB_storage[PILOTO_STORAGE_SIZE];
rBstruct_s pRBuff;

#define INTERVALO_ENTRE_MEDIDAS_SECS 5

static bool f_debug_piloto = true;

struct {
	int16_t pulsos_calculados;
	int16_t pulsos_a_aplicar;
	int16_t total_pulsos_rollback;
	int16_t pulse_counts;
	uint16_t pwidth;
	t_stepper_dir dir;

	float pRef;
	float pA;
	float pB;
	float pError;
	bool motor_running;
	uint8_t loops;
    uint8_t ch_pA;
    uint8_t ch_pB;


} PLTCB;	// Piloto Control Block

typedef enum { PLT_READ_INPUTS = 0, PLT_CHECK_CONDITIONS4ADJUST, PLT_AJUSTE, PLT_PROCESS_OUTPUT, PLT_EXIT } t_plt_states;

//Configuracion local del sistema de pilotos
piloto_conf_t piloto_conf;

bool piloto_leer_slot_actual( int8_t *slot_id );
bool FSM_ajuste_presion( float presion );
void plt_read_inputs( int8_t samples, uint16_t intervalo_secs );

// -----------------------------------------------------------------------------
void piloto_update_local_config( piloto_conf_t *piloto_system_conf)
{
    memcpy( piloto_system_conf, &piloto_conf, sizeof(piloto_conf_t));
}
// -----------------------------------------------------------------------------
void piloto_read_local_config( piloto_conf_t *piloto_system_conf)
{
    memcpy( &piloto_conf, piloto_system_conf, sizeof(piloto_conf_t)); 
}
//------------------------------------------------------------------------------
bool piloto_configurado(void)
{
    /*
     * Indica si en la confiugracion está indicado que hay un piloto
     */
    
    return(true);
}
//------------------------------------------------------------------------------
void piloto_init(void)
{
    /*
     * Usamos un ringbuffer como estructura para almacenar los pedidos de
     * ajustes de presión.
     */
    rBstruct_CreateStatic ( 
        &pRBuff,
        &pRB_storage, 
        PILOTO_STORAGE_SIZE,
        sizeof(s_pltRb_t), 
        true  
    );
    
}
//------------------------------------------------------------------------------
void piloto_productor(void)
{
    /*
     * Tenemos 3 fuentes de generar pedidos de ajustes de presion:
     * - TimeSlot
     * - Ordenes Online (vienen en las respuestas a dataframes)
     * - Ordenes de linea de comando.
     * 
     * Las ordenes online y de cmdline entran solas al ringbuffer.
     * Solo debemos ejecutar el handle de timeslots.
     * 
     */
    
    piloto_productor_handler_slots();
    
}
//------------------------------------------------------------------------------
void piloto_consumidor(void)
{
	
    /*
	 * CONSUMIDOR:
	 * Leo la presion de la FIFO y veo si puedo ajustar.
	 * Si ajusto y todo sale bien, la saco de la FIFO.
	 * Si no pude ajustar, la dejo en la FIFO para que el proximo ciclo vuelva a reintentar
	 * con esta accion pendiente.
	 */

s_pltRb_t jobOrder;
float presion;
bool borrar_request = false;

    if (f_debug_piloto) {
        xprintf_P( PSTR("PILOTO CONS: start.\r\n"));
    }

	// No hay datos en la FIFO
	if ( rBstruct_GetCount(&pRBuff) == 0 ) {
		goto quit;
	}     

    // Saco un elemento de la FIFO sin borrarlo aún
    rBstruct_PopRead(&pRBuff, &jobOrder);
    presion = jobOrder.presion;
    
    if (f_debug_piloto) {
        xprintf_P( PSTR("PILOTO CONS: new_press=%0.2f.\r\n"), presion);
    }
    
    // Ajuste de presion
    borrar_request = FSM_ajuste_presion(presion);
    
    // Borro el elemento de la FIFO si pude ejecutar el handler
	if ( borrar_request ) {
		rBstruct_Pop(&pRBuff, &jobOrder );
	}
 
quit:

    if (f_debug_piloto) {
        xprintf_P( PSTR("PILOTO CONS: end.\r\n"));
    }
    return;
}
//------------------------------------------------------------------------------
bool FSM_ajuste_presion( float presion)
{
    /*
     * Maquira de estados que ajsuta la presion
     */
    
uint8_t state = PLT_READ_INPUTS;

    PLTCB.pRef = presion;
    PLTCB.loops = 0;
    PLTCB.ch_pA = piloto_conf.ch_pA;
    PLTCB.ch_pB = piloto_conf.ch_pB;
    
    switch(state) {
    case PLT_READ_INPUTS:
        // Leo las entradas
        if (f_debug_piloto) {
            xprintf_P( PSTR("PILOTO FSMajuste: state READ_INPUTS\r\n"));
			// Espero siempre 30s antes para que se estabilizen. Sobre todo en valvulas grandes
			xprintf_P( PSTR("PILOTO FSMajuste: await 30s\r\n"));
        }
		vTaskDelay( ( TickType_t) (30000 / portTICK_PERIOD_MS ) );
		plt_read_inputs(5, INTERVALO_ENTRE_MEDIDAS_SECS );
		state = PLT_CHECK_CONDITIONS4ADJUST;
        break;
    }
   
    return(false);
}
//------------------------------------------------------------------------------
void plt_read_inputs( int8_t samples, uint16_t intervalo_secs )
{
	/*
	 * Lee las entradas: pA,pB.
	 * Medimos N veces y promediamos
	 * Dejamos el valor en gramos
	 *
	 */

uint8_t i;
float mag;
uint16_t raw;

	// Mido pA/pB
	PLTCB.pA = 0;
	PLTCB.pB = 0;
    
    ainputs_prender_sensores();
    
	for ( i = 0; i < samples; i++) {
        
        ainputs_read_channel ( PLTCB.ch_pA, &mag, &raw );
        PLTCB.pA += mag;
        if (f_debug_piloto) {
            xprintf_P(PSTR("PILOTO READINPUTS: pA:[%d]->%0.3f\r\n"), i, mag );
        }
        
        ainputs_read_channel ( PLTCB.ch_pB, &mag, &raw );
        PLTCB.pB += mag;
        if (f_debug_piloto) {
            xprintf_P(PSTR("PILOTO READINPUTS: pB:[%d]->%0.3f\r\n"), i, mag );
        }        
		
		vTaskDelay( ( TickType_t)( intervalo_secs * 1000 / portTICK_PERIOD_MS ) );
	}
    
    ainputs_apagar_sensores();

	PLTCB.pA /= samples;
	PLTCB.pB /= samples;
    if (f_debug_piloto) {
        xprintf_P(PSTR("PILOTO READINPUTS: pA=%.02f, pB=%.02f\r\n"), PLTCB.pA, PLTCB.pB );
    }
    
}
//------------------------------------------------------------------------------------
/*
 Handlers de Productor
 */
void piloto_productor_handler_slots(void)
{
    /*
     * Lo invoca el productor cada 1 minuto.
     * Se fija en que timeslot esta y si cambio o no.
     * Si cambio, manda la orden de ajustar a la presion corespondiente 
     * al nuevo tslot.
     * 
     */
    
static int8_t slot_now = -1;
int8_t slot_next = -1;
float presion;
s_pltRb_t jobOrder;

    if (f_debug_piloto) {
        xprintf_P(PSTR("PILOTO PROD: start.\r\n"));
    }
	//xprintf_P(PSTR("PILOTO: SLOTS_A: slot_actual=%d, slot=%d\r\n"), slot_actual, slot );
	if ( piloto_leer_slot_actual( &slot_next ) ) {
        if (f_debug_piloto) {
            xprintf_P( PSTR("PILOTO PROD: SLOTS slot_now=%d, slot_next=%d\r\n"), slot_now, slot_next );
		}
        // Cambio el slot. ?
		if ( slot_now != slot_next ) {
			slot_now = slot_next;
            presion = piloto_conf.pltSlots[slot_now].presion;
           
			// Un nuevo slot borra todo lo anterior.
			rBstruct_Flush(&pRBuff);

			// Guardo la presion en la cola FIFO.
            jobOrder.presion = presion;
            rBstruct_Poke(&pRBuff, &jobOrder );
    
            if (f_debug_piloto) {
                xprintf_P(PSTR("PILOTO PROD: Cambio de slot: %d=>%d, pRef=%.03f\r\n"), slot_now, slot_next, presion);
            }
		}
	}
    if (f_debug_piloto) {
        xprintf_P(PSTR("PILOTO PROD: end.\r\n"));    
    }
}
//------------------------------------------------------------------------------
void piloto_productor_handler_online( float presion)
{
    /*
     * Se invoca desde tkWAN cuando llega en una respuesta una orden
     * de actuar sobre la presion.
     */
    
s_pltRb_t jobOrder;

	xprintf_P(PSTR("PILOTO PROD: onlineOrder (p=%0.2f).\r\n"), presion);
	jobOrder.presion = presion;
    
    // Un nuevo slot borra todo lo anterior.
	rBstruct_Flush(&pRBuff);
    
	rBstruct_Poke(&pRBuff, &jobOrder );
    
}
//------------------------------------------------------------------------------
void piloto_productor_handler_cmdline( float presion)
{
    /*
     * Se invoca desde el cmdline.( para testing )
     * 
     */
  
s_pltRb_t jobOrder;

	xprintf_P(PSTR("PILOTO PROD: cmdlineOrder (p=%0.2f).\r\n"), presion);
	jobOrder.presion = presion;
    
    // Un nuevo slot borra todo lo anterior.
	rBstruct_Flush(&pRBuff);
    
	rBstruct_Poke(&pRBuff, &jobOrder );
        
}
//------------------------------------------------------------------------------
bool piloto_cmd_set_presion(char *s_presion)
{
    /*
     * Funcion invocada desde cmdline para setear una presion
     * Genera una 'orden' de trabajo para el consumidor con la presion
     * dada
     */

float presion = atof(s_presion);
    piloto_productor_handler_cmdline(presion);
    return (true);
}
//------------------------------------------------------------------------------
/*
 Configuracion
 */
void piloto_config_defaults(void)
{
    
uint8_t i;
    
    piloto_conf.enabled = false;
    piloto_conf.pWidth = 10;
    piloto_conf.pulsesXrev = 2000;
    for(i=0; i<MAX_PILOTO_PSLOTS; i++) {
        piloto_conf.pltSlots[i].presion = 0.0;
        piloto_conf.pltSlots[i].pTime = 0;
    }
    piloto_conf.ch_pA = 0;
    piloto_conf.ch_pB = 1;
    
}
//------------------------------------------------------------------------------
void piloto_print_configuration( void )
{
    
int8_t slot;

	xprintf_P( PSTR("Piloto:\r\n"));
    xprintf_P(PSTR(" debug: "));
    f_debug_piloto ? xprintf_P(PSTR("true\r\n")) : xprintf_P(PSTR("false\r\n"));

    if (  ! piloto_conf.enabled ) {
        xprintf_P( PSTR(" status=disabled\r\n"));
        return;
    }
    
    xprintf_P( PSTR(" status=enbled\r\n"));
	xprintf_P( PSTR(" pPulsosXrev=%d, pWidth=%d(ms)\r\n"), piloto_conf.pulsesXrev, piloto_conf.pWidth  );
    xprintf_P( PSTR(" ch_pA=%d, ch_pB=%d\r\n"), piloto_conf.ch_pA, piloto_conf.ch_pB);
	xprintf_P( PSTR(" Slots:\r\n"));
	xprintf_P( PSTR(" "));
	for (slot=0; slot < (MAX_PILOTO_PSLOTS / 2);slot++) {
		xprintf_P( PSTR("[%02d]%02d->%0.2f "), slot, piloto_conf.pltSlots[slot].pTime,piloto_conf.pltSlots[slot].presion  );
	}
	xprintf_P( PSTR("\r\n"));

	xprintf_P( PSTR(" "));
	for (slot=(MAX_PILOTO_PSLOTS / 2); slot < MAX_PILOTO_PSLOTS;slot++) {
		xprintf_P( PSTR("[%02d]%02d->%0.2f "), slot, piloto_conf.pltSlots[slot].pTime, piloto_conf.pltSlots[slot].presion  );
	}
	xprintf_P( PSTR("\r\n"));
}
//------------------------------------------------------------------------------
bool piloto_config_slot( uint8_t slot, char *s_ptime, char *s_presion )
{
	// Configura un slot

uint16_t ptime;
float presion;

	// Intervalos tiempo:presion:

    ptime = atoi(s_ptime);
    presion = atof(s_presion);
    
	if ( ( slot < MAX_PILOTO_PSLOTS ) && ( ptime < 2359 ) && ( presion >= 0.0 ) ){
        
        piloto_conf.pltSlots[slot].pTime = ptime;
        piloto_conf.pltSlots[slot].presion = presion;
		return(true);
	}

	return(false);
}
//------------------------------------------------------------------------------
bool piloto_config_pulseXrev( char *s_pulseXrev )
{
	piloto_conf.pulsesXrev = atoi(s_pulseXrev);
    return (true);
}
//------------------------------------------------------------------------------
bool piloto_config_pwidth( char *s_pwidth )
{
	piloto_conf.pWidth = atoi(s_pwidth);
    return(true);
}
//------------------------------------------------------------------------------
bool piloto_config_enable( char *s_enable )
{
    
    if (!strcmp_P( strupr(s_enable), PSTR("TRUE"))  ) {
        piloto_conf.enabled = true;
        return (true);
    }
    
    if (!strcmp_P( strupr(s_enable), PSTR("FALSE"))  ) {
        piloto_conf.enabled = false;
        return (true);
    }
    
    return(false);
}
//------------------------------------------------------------------------------
uint8_t piloto_hash( uint8_t f_hash(uint8_t seed, char ch )  )
    {
     /*
      * Calculo el hash de la configuracion de modbus.
      */
    
uint8_t i,j;
uint8_t hash = 0;
char *p;
uint8_t hash_buffer[64];

   // Calculo el hash de la configuracion modbus

    memset(hash_buffer, '\0', sizeof(hash_buffer) );
    j = 0;
    if ( piloto_conf.enabled ) {
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("[TRUE,%04d,%02d]"),piloto_conf.pulsesXrev, piloto_conf.pWidth);
    } else {
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("[FALSE,%04d,%02d]"),piloto_conf.pulsesXrev, piloto_conf.pWidth);
    }
    p = (char *)hash_buffer;
    while (*p != '\0') {
        hash = f_hash(hash, *p++);
    }
    //xprintf_P(PSTR("HASH_PILOTO:%s, hash=%d\r\n"), hash_buffer, hash ); 

    for(i=0; i < MAX_PILOTO_PSLOTS; i++) {
        memset(hash_buffer, '\0', sizeof(hash_buffer) );
        sprintf_P( (char *)&hash_buffer, PSTR("[S%02d:%04d,%0.2f]"), i, piloto_conf.pltSlots[i].pTime, piloto_conf.pltSlots[i].presion );
        p = (char *)hash_buffer;
        while (*p != '\0') {
            hash = f_hash(hash, *p++);
        }
        //xprintf_P(PSTR("HASH_PILOTO:%s, hash=%d\r\n"), hash_buffer, hash ); 
    }

    return(hash);
}
//------------------------------------------------------------------------------
bool piloto_leer_slot_actual( int8_t *slot_id )
{
	// Determina el id del slot en que estoy.
	// Los slots deben tener presion > 0.1

RtcTimeType_t rtcDateTime;
uint16_t now;
int8_t last_slot;
int8_t slot;
uint16_t slot_start_time;

	// Vemos si hay slots configuradas.
	// Solo chequeo el primero. DEBEN ESTAR ORDENADOS !!
	if ( piloto_conf.pltSlots[0].presion < 0.1 ) {
		// No tengo slots configurados. !!
		return(false);
	}

	// Determino la hhmm actuales
	memset( &rtcDateTime, '\0', sizeof(RtcTimeType_t));
	if ( ! RTC_read_dtime(&rtcDateTime) ) {
		xprintf_P(PSTR("PILOTO ERROR: I2C:RTC:pv_get_hhhmm_now\r\n\0"));
		return(false);
	}
	now = rtcDateTime.hour * 100 + rtcDateTime.min;

	// Hay slots configurados:
	// El ultimo slot configurado es anterior al primero en tener p=0
	last_slot = MAX_PILOTO_PSLOTS - 1;
	for ( slot = 1; slot < MAX_PILOTO_PSLOTS; slot++ ) {
		if ( piloto_conf.pltSlots[slot].presion < 0.1 ) {
			last_slot = slot - 1;
			break;
		}
	}

	// Buscamos dentro de que slot se encuentra now.
	*slot_id = last_slot;
	for ( slot = 0; slot <= last_slot; slot++ ) {
		slot_start_time = piloto_conf.pltSlots[slot].pTime;

		// Chequeo inside
		if ( now < slot_start_time ) {
			// Si estoy al ppio.
			if ( slot == 0 ) {
					*slot_id = last_slot;
			// Estoy en un slot comun
			} else {
				*slot_id = slot - 1;
			}
			break;
		}
	}
	return(true);

}     
    //------------------------------------------------------------------------------
void piloto_config_debug(bool debug )
{
    if ( debug ) {
        f_debug_piloto = true;
    } else {
        f_debug_piloto = false;
    }
    
}
//------------------------------------------------------------------------------      