/*
 * File:   tkCounters.c
 * Author: pablo
 *
 * Created on 3 de noviembre de 2022
 * 
 * Espera el timerpoll y las lee.
 * Convierte los valores en magnitudes y los publica a travez del systemVars.
 * 
 */
#include "spx2022.h"

static float l_counters[NRO_COUNTER_CHANNELS];

StaticTimer_t counters_xTimerBuffer;
TimerHandle_t counters_xTimer;

void tkCounter_start_timer( void );
void tkCounter_TimerCallback( TimerHandle_t xTimer );

//------------------------------------------------------------------------------
void tkCounters(void * pvParameters)
{

TickType_t xLastWakeTime = 0;


	while (! starting_flag )
		vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

    vTaskDelay( ( TickType_t)(500 / portTICK_PERIOD_MS ) );
	xprintf_P(PSTR("Starting tkCounters...\r\n"));
   
    counters_init( systemConf.counters_conf );
    tkCounter_start_timer();
    
	xLastWakeTime = xTaskGetTickCount();

	for( ;; )
	{
        
        kick_wdt(CNT_WDG_bp);
        
		// Espero timerpoll ms.
		vTaskDelayUntil( &xLastWakeTime, ( TickType_t)( (1000 * systemConf.timerpoll ) / portTICK_PERIOD_MS ) );
        
        counters_read(l_counters, systemConf.counters_conf );
        counters_clear();
        
        // Publico los valores en el systemVars.
        while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
  			vTaskDelay( ( TickType_t)( 1 ) );
        
        memcpy(systemVars.counters, l_counters, sizeof(l_counters));
        
        xSemaphoreGive( sem_SYSVars );
	}
}
//------------------------------------------------------------------------------
void tkCounter_start_timer( void )
{
    /*
     * Arranca el timer de base de tiempos de los contadores.
     */
            
    // Arranco los ticks
	counters_xTimer = xTimerCreateStatic ("CNTA",
			pdMS_TO_TICKS( 10 ),
			pdTRUE,
			( void * ) 0,
			tkCounter_TimerCallback,
			&counters_xTimerBuffer
			);

	xTimerStart(counters_xTimer, 10);

}
// -----------------------------------------------------------------------------
void tkCounter_TimerCallback( TimerHandle_t xTimer )
{
	// Funcion de callback de la entrada de contador A.
	// Controla el pulse_width de la entrada A
	// Leo la entrada y si esta aun en X, incremento el contador y
	// prendo el timer xTimer1X que termine el debounce.
   
uint8_t cnt = 0;
    
    for (cnt=0; cnt < NRO_COUNTER_CHANNELS; cnt++) {
        counter_FSM(cnt, systemConf.counters_conf );
    }

}
//------------------------------------------------------------------------------
