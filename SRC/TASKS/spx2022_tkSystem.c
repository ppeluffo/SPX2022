/*
 * File:   tkSystem.c
 * Author: pablo
 *
 * Espera timerPoll y luego muestra los valores de las entradas analogicas.
 * 
 */

#include "spx2022.h"

dataRcd_s dataRcd;

StaticTimer_t counters_xTimerBuffer;
TimerHandle_t counters_xTimer;

void counters_start_timer( void );
void counters_TimerCallback( TimerHandle_t xTimer );

//------------------------------------------------------------------------------
void tkSystem(void * pvParameters)
{

TickType_t xLastWakeTime = 0;
uint32_t waiting_ticks;

	while (! starting_flag )
		vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
    
    xprintf_P(PSTR("Starting tkSystem..\r\n"));
    
    counters_init( systemConf.counters_conf );
    counters_start_timer();
    
    xLastWakeTime = xTaskGetTickCount();
    // Espero solo 10s para el primer poleo ( no lo almaceno !!)
    vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
    poll_data(&dataRcd);
    xprint_dr(&dataRcd);
    
	for( ;; )
	{
        kick_wdt(SYS_WDG_bp);
        // Espero timerpoll ms.
        waiting_ticks = (uint32_t)systemConf.timerpoll * 1000 / portTICK_PERIOD_MS;
		vTaskDelayUntil( &xLastWakeTime, ( TickType_t)( waiting_ticks ));
        
        // Leo datos
        poll_data(&dataRcd); 
        // Proceso ( transmito o almaceno) frame de datos por la WAN
        WAN_process_data_rcd(&dataRcd);
        // Imprimo localmente en pantalla
        xprint_dr(&dataRcd);
	}
}
//------------------------------------------------------------------------------
dataRcd_s *get_system_dr(void)
{
    return(&dataRcd);
}
//------------------------------------------------------------------------------
bool poll_data(dataRcd_s *dataRcd)
{
    /*
     * Se encarga de leer los datos.
     * Lo hacemos aqui asi es una funcion que se puede invocar desde Cmd.
     */
bool f_status;
uint8_t channel;
float mag;
uint16_t raw;
bool retS = false;

    // Prendo los sensores
    ainputs_prender_sensores();
                
    // los valores publicados en el systemVars los leo en variables locales.
    while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
  		vTaskDelay( ( TickType_t)( 1 ) );
        
    // ANALOG: Leo los 3 canales analogicos
    for ( channel = 0; channel < NRO_ANALOG_CHANNELS; channel++) {
        ainputs_read_channel ( channel, systemConf.ainputs_conf, &mag, &raw );
        systemVars.ainputs[channel] = mag;
    }
    // Apago los sensores
    ainputs_apagar_sensores();
        
    // Leo el valor de los contadores
    counters_read( systemVars.counters, systemConf.counters_conf );
    counters_clear();
        
    // Armo el dr.
    memcpy(dataRcd->l_ainputs, systemVars.ainputs, sizeof(dataRcd->l_ainputs));
    memcpy(dataRcd->l_counters, systemVars.counters, sizeof(dataRcd->l_counters));  
    // Agrego el timestamp.
    f_status = RTC_read_dtime( &dataRcd->rtc );
    if ( ! f_status ) {
        xprintf_P(PSTR("ERROR: I2C:RTC:data_read_inputs\r\n"));
        retS = false;
        goto quit;
    }
    
    // Control de errores
    // 1- Clock:
    if ( dataRcd->rtc.year == 0) {
        xprintf_P(PSTR("DATA ERROR: byClock\r\n"));
        retS = false;
        goto quit;
    }
        
    retS = true;
        
 quit:
 
    xSemaphoreGive( sem_SYSVars );
    return(retS);
        
}
//------------------------------------------------------------------------------
void counters_start_timer( void )
{
    /*
     * Arranca el timer de base de tiempos de los contadores.
     */
            
    // Arranco los ticks
	counters_xTimer = xTimerCreateStatic ("CNTA",
			pdMS_TO_TICKS( 10 ),
			pdTRUE,
			( void * ) 0,
			counters_TimerCallback,
			&counters_xTimerBuffer
			);

	xTimerStart(counters_xTimer, 10);

}
// -----------------------------------------------------------------------------
void counters_TimerCallback( TimerHandle_t xTimer )
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
