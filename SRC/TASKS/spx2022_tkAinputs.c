/*
 * File:   tkAinputs.c
 * Author: pablo
 *
 * Created on 31 de octubre de 2022
 * 
 * Esta funcion se encarga de leer las entradas analogicas del INA3223.
 * Espera el timerpoll y las lee.
 * Convierte los valores en magnitudes y los publica a travez del systemVars.
 * 
 */
#include "spx2022.h"

static float l_ainputs[NRO_ANALOG_CHANNELS];

//------------------------------------------------------------------------------
void tkAinputs(void * pvParameters)
{

TickType_t xLastWakeTime = 0;
uint8_t channel;
float mag;
uint16_t raw;
uint32_t waiting_ticks;

	while (! starting_flag )
		vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

	xprintf_P(PSTR("Starting tkAinputs...\r\n"));

    ainputs_init();
	xLastWakeTime = xTaskGetTickCount();
    ainputs_sleep();

	for( ;; )
	{
        
        kick_wdt(AIN_WDG_bp);
        
		// Espero timerpoll ms.
        waiting_ticks = (uint32_t)systemConf.timerpoll * 1000 / portTICK_PERIOD_MS;
        vTaskDelayUntil( &xLastWakeTime, ( TickType_t)( waiting_ticks ));

		// Leo entradas analogicas
        // Prendo los sensores
        ainputs_prender_sensores();
        
        // Leo los 3 canales
        for ( channel = 0; channel < NRO_ANALOG_CHANNELS; channel++) {
            ainputs_read_channel ( channel, systemConf.ainputs_conf, &mag, &raw );
            l_ainputs[channel] = mag;
        }
        
        // Apago los sensores
        ainputs_apagar_sensores();
        
        // Publico los valores en el systemVars.
        while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 5 ) != pdTRUE )
  			vTaskDelay( ( TickType_t)( 1 ) );
        
        memcpy(systemVars.ainputs, l_ainputs, sizeof(l_ainputs));
        
        xSemaphoreGive( sem_SYSVars );
	}
}
//------------------------------------------------------------------------------
