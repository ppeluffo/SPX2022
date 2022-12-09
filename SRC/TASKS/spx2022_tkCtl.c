/*
 * File:   tkCtl.c
 * Author: pablo
 *
 * Created on 25 de octubre de 2021, 12:50 PM
 */


#include "spx2022.h"
#include "led.h"
#include "usart.h"

#define TKCTL_DELAY_S	5

void sys_watchdog_check(void);

//------------------------------------------------------------------------------
void tkCtl(void * pvParameters)
{

	// Esta es la primer tarea que arranca.

( void ) pvParameters;

	vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );
    xprintf_P(PSTR("Starting tkCtl..\r\n"));
    
    // Leo la configuracion 
    if ( ! load_config_from_NVM())  {
       xprintf_P(PSTR("Loading config default..\r\n"));
       config_default();
    }
       
    WDG_INIT();
    
    systemVars.rele_output = false;
    
    RTC_init();

    // Por ultimo habilito a todas las otras tareas a arrancar
    starting_flag = true;
    
	for( ;; )
	{
		vTaskDelay( ( TickType_t)( 1000 * TKCTL_DELAY_S / portTICK_PERIOD_MS ) );
        led_flash();
        sys_watchdog_check();
	}
}
//------------------------------------------------------------------------------
void sys_watchdog_check(void)
{
    // El watchdog se inicializa en 2F.
    // Cada tarea debe poner su bit en 0. Si alguna no puede, se resetea
    // Esta funcion se corre cada 5s (TKCTL_DELAY_S)
    
static uint8_t wdg_count = 0;

    //wdt_reset();
    //return;
        
    // EL wdg lo leo cada 60secs ( 5 x 12 )
    if ( wdg_count++ < 12 ) {
        wdt_reset();
        return;
    }
    

    // Analizo los watchdows individuales
    wdg_count = 0;
    //xprintf_P(PSTR("tkCtl: check wdg [0x%02X]\r\n"), sys_watchdog );

    if ( sys_watchdog != 0 ) {  
        xprintf_P(PSTR("tkCtl: reset by wdg [0x%02X]\r\n"), sys_watchdog );
        vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
        reset();
    } else {
        wdt_reset();
        WDG_INIT();
    }
}
//------------------------------------------------------------------------------