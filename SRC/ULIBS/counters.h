/* 
 * File:   counters.h
 * Author: pablo
 *
 * Created on 15 de septiembre de 2022, 04:46 PM
 */

#ifndef COUNTERS_H
#define	COUNTERS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
    
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
    
#include "xprintf.h"
    
#define CNT0_PORT	PORTE
#define CNT0_PIN    6   
#define CNT0_PIN_bm	PIN6_bm
#define CNT0_PIN_bp	PIN6_bp
    
#define CNT1_PORT	PORTE
#define CNT1_PIN    7
#define CNT1_PIN_bm	PIN7_bm
#define CNT1_PIN_bp	PIN7_bp
    
// Los CNTx son inputs
#define CNT0_CONFIG()    ( CNT0_PORT.DIR &= ~CNT0_PIN_bm )
#define CNT1_CONFIG()    ( CNT1_PORT.DIR &= ~CNT1_PIN_bm )


#define CNT_PARAMNAME_LENGTH	12
#define NRO_COUNTER_CHANNELS     2

typedef enum { CAUDAL = 0, PULSOS } t_counter_modo;

// Configuracion de canales de contadores
typedef struct {
	char name[CNT_PARAMNAME_LENGTH];
	float magpp;
    t_counter_modo modo_medida;
} counter_conf_t;


void counters_init( counter_conf_t *counter_conf );
void counters_config_defaults( counter_conf_t *counter_conf );
void counters_config_print(counter_conf_t *counter_conf );
bool counters_config_channel( uint8_t channel, counter_conf_t *counter_conf, char *s_name, char *s_magpp, char *s_modo );
void counters_config_debug(bool debug );
bool counters_read_debug(void);
void counter_FSM(uint8_t i, counter_conf_t *counter_conf );
void counters_clear(void);
uint8_t counters_read_pin(uint8_t cnt);
void counters_read( float *l_counters, counter_conf_t *counter_conf );
uint8_t CNT0_read(void);
uint8_t CNT1_read(void);


#ifdef	__cplusplus
}
#endif

#endif	/* COUNTERS_H */

