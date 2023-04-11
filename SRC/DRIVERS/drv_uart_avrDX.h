/*
 * drv_uart_spx.h
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#ifndef SRC_SPX_DRIVERS_DRV_UART_SPX_H_
#define SRC_SPX_DRIVERS_DRV_UART_SPX_H_

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "FreeRTOS.h"

#include "ringBuffer.h"


#ifndef F_CPU
#define F_CPU 24000000
#endif

#define USART_SET_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5);

//-----------------------------------------------------------------------
#define UART0_TXSIZE	8	// trasmito por poleo. Si uso interrupcion lo subo a 128
uint8_t uart0_txBuffer[UART0_TXSIZE];
#define UART0_RXSIZE	255	// Este UART es el que atiende el modem.
uint8_t uart0_rxBuffer[UART0_RXSIZE];
rBchar_s TXRB_uart0, RXRB_uart0;
void drv_uart0_init(uint32_t baudrate );

#define UART1_TXSIZE	8	// trasmito por poleo. Si uso interrupcion lo subo a 128
uint8_t uart1_txBuffer[UART1_TXSIZE];
#define UART1_RXSIZE	64	// 
uint8_t uart1_rxBuffer[UART1_RXSIZE];
rBchar_s TXRB_uart1, RXRB_uart1;
void drv_uart1_init(uint32_t baudrate );

#define UART2_TXSIZE	8	// trasmito por poleo. Si uso interrupcion lo subo a 128
uint8_t uart2_txBuffer[UART2_TXSIZE];
#define UART2_RXSIZE	64	// 
uint8_t uart2_rxBuffer[UART2_RXSIZE];
rBchar_s TXRB_uart2, RXRB_uart2;
void drv_uart2_init(uint32_t baudrate );

#define UART3_TXSIZE	8	// trasmito por poleo. Si uso interrupcion lo subo a 128
uint8_t uart3_txBuffer[UART3_TXSIZE];
#define UART3_RXSIZE	64	// 
uint8_t uart3_rxBuffer[UART3_RXSIZE];
rBchar_s TXRB_uart3, RXRB_uart3;
void drv_uart3_init(uint32_t baudrate );

//-----------------------------------------------------------------------


#endif /* SRC_SPX_DRIVERS_DRV_UART_SPX_H_ */
