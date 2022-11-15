/* 
 * File:   lora.h
 * Author: pablo
 *
 * Created on 15 de febrero de 2022, 04:05 PM
 */

#ifndef LORA_H
#define	LORA_H

#ifdef	__cplusplus
extern "C" {
#endif

//#include "xc.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "stdint.h"
#include "FreeRTOS.h"
#include "task.h"
#include "frtos-io.h"
#include "xprintf.h"    
#include "stdbool.h"
#include <stdio.h>
#include "linearBuffer.h"

    
#define LORA_RTS_PORT	PORTE
#define LORA_RTS_PIN_bm	PIN4_bm
#define LORA_RTS_PIN_bp	PIN4_bp

#define LORA_CTS_PORT	PORTE
#define LORA_CTS_PIN_bm	PIN3_bm
#define LORA_CTS_PIN_bp	PIN3_bp
    
#define LORA_RESET_PORT     PORTE
#define LORA_RESET_PIN_bm	PIN2_bm
#define LORA_RESET_PIN_bp	PIN2_bp
   
// RTS es input
#define LORA_RTS_CONFIG()    ( LORA_RTS_PORT.DIR &= ~LORA_RTS_PIN_bm )
uint8_t lora_read_rts(void);

// CTS, RESET son outputs
#define LORA_CTS_CONFIG()    ( LORA_CTS_PORT.DIR |= LORA_CTS_PIN_bm )
#define SET_LORA_CTS()       ( LORA_CTS_PORT.OUT |= LORA_CTS_PIN_bm )
#define CLEAR_LORA_CTS()     ( LORA_CTS_PORT.OUT &= ~LORA_CTS_PIN_bm )
#define TOGGLE_LORA_CTS()    ( LORA_CTS_PORT.OUT ^= 1UL << LORA_CTS_PIN_bp);
    
#define LORA_RESET_CONFIG()    ( LORA_RESET_PORT.DIR |= LORA_RESET_PIN_bm )
#define SET_LORA_RESET()       ( LORA_RESET_PORT.OUT |= LORA_RESET_PIN_bm )
#define CLEAR_LORA_RESET()     ( LORA_RESET_PORT.OUT &= ~LORA_RESET_PIN_bm )
#define TOGGLE_LORA_RESET()    ( LORA_RESET_PORT.OUT ^= 1UL << LORA_RESET_PIN_bp)

#define lora_reset_on()     SET_LORA_RESET()
#define lora_reset_off()    CLEAR_LORA_RESET()


#define LORA_RX_DECODED_BUFFER_SIZE 128


lBuffer_s lora_rx_sdata;
#define LORA_RX_BUFFER_SIZE 128
char lora_rx_buffer[LORA_RX_BUFFER_SIZE];


lBuffer_s lora_tx_sdata;
#define LORA_TX_BUFFER_SIZE 128
char lora_tx_buffer[LORA_TX_BUFFER_SIZE];

lBuffer_s lora_decoded_rx_sdata;
#define LORA_DECODED_RX_BUFFER_SIZE 64
char lora_decoded_rx_buffer[LORA_DECODED_RX_BUFFER_SIZE];

SemaphoreHandle_t sem_LORA;
StaticSemaphore_t LORA_xMutexBuffer;
#define MSTOTAKELORASEMPH ((  TickType_t ) 10 )


struct {
    bool rsp_ok;
    bool rsp_invalid_params;
    bool rsp_busy;
    bool rsp_radio_rx;
    bool rsp_radio_tx_ok;
    bool rsp_radio_err;
} radio_responses;


void LORA_init(void);
void lora_flash_led(void);
bool lora_send_msg_in_ascii(char *msg);
void lora_send_cmd(char **argv );
void lora_responses_clear(void);
void lora_responses_print(void);
void lora_decode_msg(void);
void lora_print_rxvd_msg(void);
void lora_read_snr(void);
void lora_send_confirmation(void);


#ifdef	__cplusplus
}
#endif

#endif	/* LORA_H */

