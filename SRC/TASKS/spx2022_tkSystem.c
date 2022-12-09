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
        
        // Transmito frame de datos por la WAN
        WAN_xmit_data_frame(&dataRcd);
        
        // Imprimo localmente en pantalla
        xprint_terminal(XPRINT_HEADER);
            
        kick_wdt(SYS_WDG_bp);
           
	}
}
//------------------------------------------------------------------------------
void xprint_terminal(bool print_header)
{
uint8_t channel;
bool start = true;

    if (print_header) {
        xprintf_P( PSTR("(%s) ID:%s;TYPE:%s;VER:%s;"), RTC_logprint(FORMAT_SHORT), systemConf.dlgid, FW_TYPE, FW_REV);
    }
    
    // Analog Channels:
    for ( channel=0; channel < NRO_ANALOG_CHANNELS; channel++) {
        if ( strcmp ( systemConf.ainputs_conf[channel].name, "X" ) != 0 ) {
            if ( start ) {
                xprintf_P( PSTR("%s:%0.2f"), systemConf.ainputs_conf[channel].name, dataRcd.l_ainputs[channel]);
                start = false;
            } else {
                xprintf_P( PSTR(";%s:%0.2f"), systemConf.ainputs_conf[channel].name, dataRcd.l_ainputs[channel]);
            }
        }
    }
        
    // Counter Channels:
    for ( channel=0; channel < NRO_COUNTER_CHANNELS; channel++) {
        if ( strcmp ( systemConf.counters_conf[channel].name, "X" ) != 0 ) {
            if ( start ) {
                xprintf_P( PSTR("%s:%0.3f"), systemConf.counters_conf[channel].name, dataRcd.l_counters[channel]);
                start = false;
            } else {
                xprintf_P( PSTR(";%s:%0.3f"), systemConf.counters_conf[channel].name, dataRcd.l_counters[channel]);
            }
        }
    }
    xprintf_P( PSTR("\r\n"));
}
//------------------------------------------------------------------------------
