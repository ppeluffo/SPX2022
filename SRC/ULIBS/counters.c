/*
 * https://stackoverflow.com/questions/13730786/c-pass-int-array-pointer-as-parameter-into-a-function
 *  
 */
#include "counters.h"

typedef enum { P_OUTSIDE=0, P_CHECKING, P_INSIDE, P_FILTER } pulse_t;

// Estructura de control de los contadores.
typedef struct {
	float caudal;                    // Caudal instantaneo en c/pulso
	uint32_t ticks_count;            // total de ticks entre el pulso actual y el anterior
    uint8_t  debounceTicks_count;    // ticks desde el flanco del pulso a validarlos
    uint16_t  filterTicks_count;     // ticks desde que lo valido hasta que permito que venga otro.   
	uint16_t pulse_count;            // contador de pulsos
	pulse_t state;                   // variable de estado que indica si estoy dentro o fuera de un pulso
} counter_cbk_t;

static counter_cbk_t CNTCB[NRO_COUNTER_CHANNELS];
static bool f_debug_counters;

// Los caudales los almaceno en un RB y lo que doy es el promedio !!
typedef struct {
    float caudal;
} t_caudal_s;

#define MAX_RB_CAUDAL_STORAGE_SIZE  5
t_caudal_s caudal_storage_0[MAX_RB_CAUDAL_STORAGE_SIZE];
t_caudal_s caudal_storage_1[MAX_RB_CAUDAL_STORAGE_SIZE];
rBstruct_s caudal_RB_0,caudal_RB_1;

void promediar_rb_caudal(void);

// -----------------------------------------------------------------------------
void counters_init( counter_conf_t *counters_conf )
{
    
uint8_t i;

    CNT0_CONFIG();
    CNT1_CONFIG();
            
    f_debug_counters = false;
       
    for ( i=0;i<NRO_COUNTER_CHANNELS;i++) {
		CNTCB[i].caudal = 0.0;          
		CNTCB[i].ticks_count = 0;
        CNTCB[i].debounceTicks_count = 0;
        CNTCB[i].filterTicks_count = 0;
		CNTCB[i].pulse_count = 0;
		CNTCB[i].state = P_OUTSIDE;
	}

    counters_clear();
    
    rBstruct_CreateStatic ( 
        &caudal_RB_0, 
        &caudal_storage_0, 
        MAX_RB_CAUDAL_STORAGE_SIZE, 
        sizeof(t_caudal_s), 
        true  
    );

    rBstruct_CreateStatic ( 
        &caudal_RB_1, 
        &caudal_storage_1, 
        MAX_RB_CAUDAL_STORAGE_SIZE, 
        sizeof(t_caudal_s), 
        true  
    );
}
// -----------------------------------------------------------------------------
uint8_t CNT0_read(void)
{
    return ( ( CNT0_PORT.IN & CNT0_PIN_bm ) >> CNT0_PIN) ;
}
// -----------------------------------------------------------------------------
uint8_t CNT1_read(void)
{
    return ( ( CNT1_PORT.IN & CNT1_PIN_bm ) >> CNT1_PIN) ;
}
// -----------------------------------------------------------------------------
void counters_config_defaults( counter_conf_t *counters_conf )
{
    /*
     * Realiza la configuracion por defecto de los canales digitales.
     */

uint8_t i = 0;

	for ( i = 0; i < NRO_COUNTER_CHANNELS; i++ ) {
		//snprintf_P( counters_conf[i].name, CNT_PARAMNAME_LENGTH, PSTR("X%d\0"),i );
        snprintf_P( counters_conf[i].name, CNT_PARAMNAME_LENGTH, PSTR("X") );
		counters_conf[i].magpp = 1;
        counters_conf[i].modo_medida = CAUDAL;
	}
}
//------------------------------------------------------------------------------
void counters_config_print( counter_conf_t *counters_conf )
{
    /*
     * Muestra la configuracion de todos los canales de contadores en la terminal
     * La usa el comando tkCmd::status.
     */
    
uint8_t i = 0;

    xprintf_P(PSTR("Counters:\r\n"));
    xprintf_P(PSTR(" debug: "));
    f_debug_counters ? xprintf_P(PSTR("true\r\n")) : xprintf_P(PSTR("false\r\n"));
    
	for ( i = 0; i < NRO_COUNTER_CHANNELS; i++) {
        
        xprintf_P( PSTR(" c%d: [%s,magpp=%.03f,"), i, counters_conf[i].name, counters_conf[i].magpp );
        if ( counters_conf[i].modo_medida == CAUDAL ) {
            xprintf_P(PSTR("CAUDAL]\r\n"));
        } else {
            xprintf_P(PSTR("PULSO]\r\n"));
        }
	}

            
}
//------------------------------------------------------------------------------
bool counters_config_channel( uint8_t channel, counter_conf_t *counters_conf, char *s_name, char *s_magpp, char *s_modo )
{
	// Configuro un canal contador.
	// channel: id del canal
	// s_param0: string del nombre del canal
	// s_param1: string con el valor del factor magpp.
	//
	// {0..1} dname magPP

bool retS = false;

	if ( s_name == NULL ) {
		return(retS);
	}

	if ( ( channel >=  0) && ( channel < NRO_COUNTER_CHANNELS ) ) {

		// NOMBRE
		snprintf_P( counters_conf[channel].name, CNT_PARAMNAME_LENGTH, PSTR("%s"), s_name );

		// MAGPP
		if ( s_magpp != NULL ) { counters_conf[channel].magpp = atof(s_magpp); }

        // MODO ( PULSO/CAUDAL )
		if ( s_modo != NULL ) {
			if ( strcmp_P( strupr(s_modo), PSTR("PULSO")) == 0 ) {
				counters_conf[channel].modo_medida = PULSOS;

			} else if ( strcmp_P( strupr(s_modo) , PSTR("CAUDAL")) == 0 ) {
				counters_conf[channel].modo_medida = CAUDAL;

			} else {
				xprintf_P(PSTR("ERROR: counters modo: PULSO/CAUDAL only!!\r\n"));
			}
		}
        
		retS = true;
	}

	return(retS);

}
//------------------------------------------------------------------------------
void counters_config_debug(bool debug )
{
    if ( debug ) {
        f_debug_counters = true;
    } else {
        f_debug_counters = false;
    }
}
//------------------------------------------------------------------------------
bool counters_read_debug(void)
{
    return (f_debug_counters);
}
//------------------------------------------------------------------------------
uint8_t counter_read_pin(uint8_t cnt)
{
    switch(cnt) {
        case 0:
            return(CNT0_read());
            break;
        case 1:
            return(CNT1_read());
            break;
        default:
            return(0);
            
    }
    return(0);
}
//------------------------------------------------------------------------------
void counter_FSM(uint8_t i, counter_conf_t *counters_conf )
{

uint16_t duracion_pulso_ticks = 0;
t_caudal_s rb_element;
 
    // Esta funcion la invoca el timerCallback. c/vez sumo 1 para tener los ticks
    // desde el pulso anterior.
	CNTCB[i].ticks_count++;
    
    switch ( CNTCB[i].state ) {
        case P_OUTSIDE:
            if ( counter_read_pin(i) == 0 ) {   // Llego un flanco. Inicializo
                CNTCB[i].state = P_CHECKING;
                CNTCB[i].debounceTicks_count = 0;
                CNTCB[i].filterTicks_count = 0;
                return;                
            }
            break;
            
        case P_CHECKING:
            if ( counter_read_pin(i) == 1 ) {   // Falso pulso. Me rearmo y salgo
                CNTCB[i].state = P_OUTSIDE;
                return;
            }
            // Controlo el periodo de debounce. (2ticks = 20ms)
            CNTCB[i].debounceTicks_count++;
            if ( CNTCB[i].debounceTicks_count == 2 ) {   
                CNTCB[i].pulse_count++;     // Pulso valido
                
                // Calculo el caudal instantaneo
                if ( counters_conf[i].modo_medida == CAUDAL ) {
                    // Tengo 1 pulso en N ticks.
                    // 1 pulso -------> ticks_counts * 10 mS
                    // magpp (mt3) ---> ticks_counts * 10 mS
                    duracion_pulso_ticks = CNTCB[i].ticks_count;
                    CNTCB[i].ticks_count = 0;
                    
                    if ( duracion_pulso_ticks > 0 ) {
                        CNTCB[i].caudal =  (( counters_conf[i].magpp * 3600000) /  ( duracion_pulso_ticks * 10)  ); // En mt3/h  
                    } else {   
                        CNTCB[i].caudal = 0;
                    } 
                    // Guardo el caudal en el RB
                    rb_element.caudal = CNTCB[i].caudal;
                    if (i==0) {
                        rBstruct_Poke(&caudal_RB_0, &rb_element);
                    } else {
                        rBstruct_Poke(&caudal_RB_1, &rb_element);
                    }
                    
                }
                
                CNTCB[i].state = P_INSIDE;  // Paso a esperar que suba
                
                if ( f_debug_counters ) {
                    if ( counters_conf[i].modo_medida == CAUDAL ) {
                        xprintf_P( PSTR("COUNTERS: Q%d=%0.3f, P=%d, ticks=%d\r\n"), i, CNTCB[i].caudal, CNTCB[i].pulse_count, duracion_pulso_ticks );
                    } else {
                        xprintf_P( PSTR("COUNTERS: P%d=%d\r\n"), i, CNTCB[i].pulse_count );
                    }
                } 
                return;
            }
            break;
            
        case P_INSIDE:
            if ( counter_read_pin(i) == 0 ) {   // Flanco: Fin de pulso; Me rearmo y salgo
                CNTCB[i].state = P_FILTER;
                CNTCB[i].filterTicks_count = 0;
                return;
            }
            break;
            
        case P_FILTER:
            // Debo esperar al menos 10 ticks (100ms) antes de contar de nuevo
             CNTCB[i].filterTicks_count++;
            if ( (  CNTCB[i].filterTicks_count > 10 ) && ( counter_read_pin(i) == 1 ) ) {   
                CNTCB[i].state = P_OUTSIDE;
                return;
            }
            break;
            
        default:
            // Error: inicializo
            CNTCB[i].caudal = 0.0;          
            CNTCB[i].ticks_count = 0;
            CNTCB[i].debounceTicks_count = 0;
            CNTCB[i].filterTicks_count = 0;
            CNTCB[i].pulse_count = 0;
            CNTCB[i].state = P_OUTSIDE;
            return;
    }

}
//------------------------------------------------------------------------------
void counters_convergencia(void)
{
    /*
     * Esta opcion es para asegurar la convergencia del valor
     * en la medida que el caudal es 0.
     * En este caso debemos insertar un registro en el ringbuffer en 0 si 
     * en el periodo no llegaron pulsos.
     * OJO: Esto va antes de poner los contadores en 0. !!!
     */
  
uint8_t cnt;
t_caudal_s rb_element;

    //xprintf_P(PSTR("DEBUG COUNTERS CLEAR\r\n"));
    for ( cnt=0; cnt < NRO_COUNTER_CHANNELS; cnt++) {
        
        // Aseguro la convergencia a 0 de los caudales
        if ( CNTCB[cnt].pulse_count == 0 ) {
            rb_element.caudal = 0.0;
            if (cnt==0) {
                rBstruct_insert_at_tail( &caudal_RB_0, &rb_element );
            } else {
                rBstruct_insert_at_tail( &caudal_RB_1, &rb_element );
            }
        }
    }

}
//------------------------------------------------------------------------------
void counters_clear(void)
{
   /*
    * Una vez por periodo ( timerpoll ) borro los contadores.
    * Si en el periodo NO llegaron pulsos, aqui debo entonces en los
    * caudales agregar un 0.0 al ring buffer para que luego de un tiempo
    * converja a 0.
    * 
    */
    
uint8_t cnt;

    //xprintf_P(PSTR("DEBUG COUNTERS CLEAR\r\n"));
    for ( cnt=0; cnt < NRO_COUNTER_CHANNELS; cnt++) {        
        CNTCB[cnt].pulse_count = 0;
        CNTCB[cnt].caudal = 0.0;
    }
}
//------------------------------------------------------------------------------
void counters_read( float *l_counters, counter_conf_t *counters_conf )
{

uint8_t i;
    
    promediar_rb_caudal();
    
    for (i=0; i < NRO_COUNTER_CHANNELS; i++) {
        
        //xprintf_P( PSTR("DEBUG1: C%d=%d\r\n"), i, CNTCB[i].pulse_count );

        if ( counters_conf[i].modo_medida == CAUDAL ) {
            l_counters[i] = CNTCB[i].caudal;
        } else {
            l_counters[i] = (float) CNTCB[i].pulse_count;
        }

    }
    
}
//------------------------------------------------------------------------------
void promediar_rb_caudal(void)
{
uint8_t i;
t_caudal_s rb_element;
float q0,q1, Qavg0,Qavg1;

    // Promedio los ringBuffers
    Qavg0=0.0;
    Qavg1=0.0;

    for (i=0; i<MAX_RB_CAUDAL_STORAGE_SIZE; i++) {
        rb_element = caudal_storage_0[i];
        q0 = rb_element.caudal;
        Qavg0 += q0;
        rb_element = caudal_storage_1[i];
        q1 = rb_element.caudal;
        Qavg1 += q1;
        if ( f_debug_counters ) {
            xprintf_P(PSTR("DEBUG: i=%d [q0=%0.3f, avgQ0=%0.3f] [q1=%0.3f, avgQ1=%0.3f]\r\n"), i, q0, Qavg0,q1,Qavg1);
        }
    }
    
    Qavg0 /= MAX_RB_CAUDAL_STORAGE_SIZE;
    Qavg1 /= MAX_RB_CAUDAL_STORAGE_SIZE;
    CNTCB[0].caudal = Qavg0;
    CNTCB[1].caudal = Qavg1;
    
    if ( f_debug_counters ) {
        xprintf_P(PSTR("DEBUG: Qavg0=%0.3f, Qavg1=%0.3f\r\n"), Qavg0,Qavg1);
    }

}
//------------------------------------------------------------------------------
void counters_test_rb(char *data)
{
    
    //guardo el data en el RB, lo promedio, imprimo el RB y el promedio
    
/*
t_caudal_s rb_element;
uint8_t i;
float avg;

    rb_element.caudal = atof(data);
    rBstruct_Poke(&caudal_RB, &rb_element);

    avg = 0.0;
    for (i=0; i<MAX_RB_CAUDAL_STORAGE_SIZE; i++) {
        rb_element = caudal_storage[i];
        avg += rb_element.caudal;
        xprintf_P(PSTR("DEBUG: i=%d, element=%d, avg=%0.3f\r\n"), i, rb_element.caudal, avg);
    }
    avg /= MAX_RB_CAUDAL_STORAGE_SIZE;
    xprintf_P(PSTR("DEBUG: AVG=%0.3f\r\n"), avg);
*/
    
}

        