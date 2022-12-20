/*
 * File:   tkSystem.c
 * Author: pablo
 *
 * Espera timerPoll y luego muestra los valores de las entradas analogicas.
 * 
 */

#include "spx2022.h"

dataRcd_s dataRcd;

//------------------------------------------------------------------------------
void tkSystem(void * pvParameters)
{

TickType_t xLastWakeTime = 0;
uint32_t waiting_ticks;

	while (! starting_flag )
		vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
    
    xprintf_P(PSTR("Starting tkSystem..\r\n"));
    
    xLastWakeTime = xTaskGetTickCount();
    
    vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
    poll_data();
    
	for( ;; )
	{
        kick_wdt(SYS_WDG_bp);
        // Espero timerpoll ms.
        waiting_ticks = (uint32_t)systemConf.timerpoll * 1000 / portTICK_PERIOD_MS;
		vTaskDelayUntil( &xLastWakeTime, ( TickType_t)( waiting_ticks ));
        
        poll_data();      
	}
}
//------------------------------------------------------------------------------
dataRcd_s *get_system_dr(void)
{
    return(&dataRcd);
}
//------------------------------------------------------------------------------
void poll_data(void)
{
    /*
     * Se encarga de leer los datos.
     * Lo hacemos aqui asi es una funcion que se puede invocar desde Cmd.
     */
bool f_status;

        // los valores publicados en el systemVars los leo en variables locales.
        while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
  			vTaskDelay( ( TickType_t)( 1 ) );
        
        memcpy(dataRcd.l_ainputs, systemVars.ainputs, sizeof(dataRcd.l_ainputs));
        memcpy(dataRcd.l_counters, systemVars.counters, sizeof(dataRcd.l_counters));
       
        // Agrego el timestamp.
        f_status = RTC_read_dtime( &dataRcd.rtc );
        if ( ! f_status )
            xprintf_P(PSTR("ERROR: I2C:RTC:data_read_inputs\r\n"));
            
        xSemaphoreGive( sem_SYSVars );
        
        // Control de errores
        // 1- Clock:
        if ( dataRcd.rtc.year == 0) {
            xprintf_P(PSTR("DATA ERROR: byClock\r\n"));
            return;
        }
        
        // Proceso ( transmito o almaceno) frame de datos por la WAN
        WAN_process_data_rcd(&dataRcd);
        
        // Imprimo localmente en pantalla
        xprint_dr(&dataRcd);

}
//------------------------------------------------------------------------------