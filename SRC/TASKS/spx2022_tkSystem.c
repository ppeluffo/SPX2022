/*
 * File:   tkSystem.c
 * Author: pablo
 *
 * Espera timerPoll y luego muestra los valores de las entradas analogicas.
 * 
 */

#include "spx2022.h"

struct {
    float l_ainputs[NRO_ANALOG_CHANNELS];
    float l_counters[NRO_COUNTER_CHANNELS];
} dataRcd;

//------------------------------------------------------------------------------
void tkSystem(void * pvParameters)
{

TickType_t xLastWakeTime = 0;
uint8_t channel;

	while (! starting_flag )
		vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
    
    xprintf_P(PSTR("Starting tkSystem..\r\n"));
    
    xLastWakeTime = xTaskGetTickCount();
    
	for( ;; )
	{
        kick_wdt(SYS_WDG_bp);
        
        // Espero timerpoll ms.
		vTaskDelayUntil( &xLastWakeTime, ( TickType_t)( (1000 * systemConf.timerpoll ) / portTICK_PERIOD_MS ) );

        // los valores publicados en el systemVars los leo en variables locales.
        while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
  			vTaskDelay( ( TickType_t)( 1 ) );
        
        memcpy(dataRcd.l_ainputs, systemVars.ainputs, sizeof(dataRcd.l_ainputs));
        memcpy(dataRcd.l_counters, systemVars.counters, sizeof(dataRcd.l_counters));
        
        xSemaphoreGive( sem_SYSVars );
        
        // Imprimo
        // Header:
        xprintf_P(PSTR("ID:%s;TYPE:%s;VER:%s;"), systemConf.dlgid, FW_TYPE, FW_REV);
        xfprintf_P( fdRS485B, PSTR("ID:%s;TYPE:%s;VER:%s;"), systemConf.dlgid, FW_TYPE, FW_REV);
        
        // Analog Channels:
        for ( channel=0; channel < NRO_ANALOG_CHANNELS; channel++) {
            if ( channel == 0 ) {
                xprintf_P(PSTR("%s:%0.3f"), systemConf.ainputs_conf[channel].name, dataRcd.l_ainputs[channel]);
                xfprintf_P( fdRS485B, PSTR("%s:%0.3f"), systemConf.ainputs_conf[channel].name, dataRcd.l_ainputs[channel]);
            } else {
                xprintf_P(PSTR(";%s:%0.3f"), systemConf.ainputs_conf[channel].name, dataRcd.l_ainputs[channel]);
                xfprintf_P( fdRS485B, PSTR(";%s:%0.3f"), systemConf.ainputs_conf[channel].name, dataRcd.l_ainputs[channel]);
            }
            
        }
        
        // Counter Channels:
        for ( channel=0; channel < NRO_COUNTER_CHANNELS; channel++) {
            xprintf_P(PSTR(";%s:%0.3f"), systemConf.counters_conf[channel].name, dataRcd.l_counters[channel]);
            xfprintf_P( fdRS485B, PSTR(";%s:%0.3f"), systemConf.counters_conf[channel].name, dataRcd.l_counters[channel]);
        }
        xprintf_P(PSTR("\r\n"));
        xfprintf_P( fdRS485B, PSTR("\r\n"));
                
        kick_wdt(SYS_WDG_bp);
           
	}
}
//------------------------------------------------------------------------------
