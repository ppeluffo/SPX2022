/*
 * File:   tkSystem.c
 * Author: pablo
 *
 * Espera timerPoll y luego muestra los valores de las entradas analogicas.
 * 
 */


#include "spx2022.h"

dataRcd_s dataRcd, dataRcd_previo;

StaticTimer_t counters_xTimerBuffer;
TimerHandle_t counters_xTimer;

void counters_start_timer( void );
void counters_TimerCallback( TimerHandle_t xTimer );

void check_alarms(dataRcd_s *dataRcd);

//------------------------------------------------------------------------------
void tkSystem(void * pvParameters)
{

//TickType_t xLastWakeTime = 0;
uint32_t waiting_ticks;
uint8_t i;

	while (! starting_flag )
		vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
    
    xprintf_P(PSTR("Starting tkSystem..\r\n"));
    
    counters_init();
    counters_start_timer();
    
    ainputs_init(systemConf.samples_count);
    
//    xLastWakeTime = xTaskGetTickCount();
    // Espero solo 10s para el primer poleo ( no lo almaceno !!)
    vTaskDelay( ( TickType_t)( 10000 / portTICK_PERIOD_MS ) );
    poll_data(&dataRcd);
    xprint_dr(&dataRcd);
    
	for( ;; )
	{
        kick_wdt(SYS_WDG_bp);
        // Espero timerpoll ms.
        //waiting_ticks = (uint32_t)systemConf.timerpoll * 1000 / portTICK_PERIOD_MS;
        //vTaskDelayUntil( &xLastWakeTime, ( TickType_t)( waiting_ticks ));
        // El poleo se lleva 5 secs.
        waiting_ticks = (uint32_t) ( systemConf.timerpoll * 1000 - PWRSENSORES_SETTLETIME_MS )  / portTICK_PERIOD_MS / 10;
        for (i=0; i< 10; i++) {
            kick_wdt(SYS_WDG_bp);
            vTaskDelay( ( TickType_t)( waiting_ticks ) );
        }
           
        // Leo datos
        poll_data(&dataRcd); 
        // Proceso ( transmito o almaceno) frame de datos por la WAN
        WAN_process_data_rcd(&dataRcd);
        // Imprimo localmente en pantalla
        xprint_dr(&dataRcd);

        // Vemos si algun valor excede el nivel de alarma
        //check_alarms(&dataRcd);
	}
}
//------------------------------------------------------------------------------
void check_alarms(dataRcd_s *dataRcd)
{
    /*
     * Compara el valor de los canales analogicos configurados con el 
     * valor ultimo transmitido.
     * Si el valor de alguno excede el nivel de alarma, indica al modem
     * que despierte.
     */
    
float valor_previo, valor_actual, delta;
uint8_t i;
bool send_signal = false;

        
    for (i=0; i < NRO_ANALOG_CHANNELS; i++) {
        valor_previo = dataRcd_previo.l_ainputs[i];
        valor_actual = dataRcd->l_ainputs[i];
        delta = systemConf.ainputs_conf.channel[i].mmax * systemConf.alarm_level / 100;
        
        //xprintf_P(PSTR("DEBUG ALARM: %d, vp=%0.3f, va=%0.3f, delta=%0.3f\r\n"), i, valor_previo, valor_actual, delta);
        
        // Si el nivel de alarma esta configurado
        if ( delta > 0) {

            // Si el canal esta configurado
            if ( strcmp ( systemConf.ainputs_conf.channel[i].name, "X" ) != 0 ) {
                
                // Si el error excede el nivel de alarma
                if ( delta < fabs( valor_actual - valor_previo) ) {
                    send_signal = true;
                }
            }
        }
    }

    if ( send_signal ) {
        xprintf_P(PSTR("ALARM: SEND WAKEUP SIGNAL\r\n"));
        while ( xTaskNotify(xHandle_tkWAN, SGN_FRAME_READY , eSetBits ) != pdPASS ) {
			vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
		}
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
        if ( systemConf.ainputs_conf.channel[channel].enabled ) {
            ainputs_read_channel ( channel, &mag, &raw );
            systemVars.ainputs[channel] = mag;
        }
    }
        
    // Leo la bateria
    if ( systemConf.ainputs_conf.channel[2].enabled ) {
        systemVars.battery = -1.0;
    } else{
        ainputs_read_channel ( 99, &mag, &raw );
        systemVars.battery = mag;
    }
    
    // Apago los sensores
    ainputs_apagar_sensores(); 
    
    // Leo el valor de los contadores
    counters_read( systemVars.counters );
    counters_convergencia();
    counters_clear();
       
    // Leo los canales modbus 
    modbus_read ( systemVars.modbus );
    
    // Armo el dr.
    memcpy(dataRcd->l_ainputs, systemVars.ainputs, sizeof(dataRcd->l_ainputs));
    memcpy(dataRcd->l_counters, systemVars.counters, sizeof(dataRcd->l_counters)); 
    memcpy(dataRcd->l_modbus, systemVars.modbus, sizeof(dataRcd->l_modbus)); 
    dataRcd->battery = systemVars.battery;  
    
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
        counter_FSM(cnt);
    }

}
//------------------------------------------------------------------------------
