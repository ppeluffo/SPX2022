/*
 * usart.c
 *
 * Created: 21/10/2020 12:21:17
 *  Author: Pablo
 */ 

#include "clock_config.h"
#include "stdbool.h"
#include "string.h"
#include <avr/wdt.h>

#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5);
#define USART4_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5);
// ---------------------------------------------------------------
void USART0_init(void)
{
	// Configuro para 115200
	
	PORTA.DIR &= ~PIN1_bm;
	PORTA.DIR |= PIN0_bm;
	
	USART0.BAUD = (uint16_t)USART0_BAUD_RATE(9600);
	USART0.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}
// ---------------------------------------------------------------
void USART0_sendChar(char c)
{
	while (!(USART0.STATUS & USART_DREIF_bm))
	{
		;
	}
	USART0.TXDATAL = c;	
}
// ---------------------------------------------------------------
void USART0_sendString(char *str)
{
	for(size_t i = 0; i < strlen(str); i++)
	{
		USART0_sendChar(str[i]);
	}
}
// ---------------------------------------------------------------
char USART0_readChar(bool echo)
{
char c;

	while (!(USART0.STATUS & USART_RXCIF_bm))
	{
		wdt_reset();
		;
	}
	c = USART0.RXDATAL;
	if ( echo)
		USART0_sendChar(c);
	return(c);
}
// ---------------------------------------------------------------
bool USART0_getChar( char *c )
{

	if ( USART0.STATUS & USART_RXCIF_bm) {
		*c = USART0.RXDATAL;
		return(true);
	}
	return(false);
}
// ---------------------------------------------------------------

void USART2_sendChar(char c)
{
	while (!(USART2.STATUS & USART_DREIF_bm))
	{
		;
	}
	USART2.TXDATAL = c;	
}
// ---------------------------------------------------------------
char USART2_readChar(bool echo)
{
char c;

	while (!(USART2.STATUS & USART_RXCIF_bm))
	{
		wdt_reset();
		;
	}
	c = USART2.RXDATAL;
	if ( echo)
		USART2_sendChar(c);
	return(c);
}
// ---------------------------------------------------------------

void USART3_init(void)
{
	// Configuro para 115200
	
	PORTB.DIR &= ~PIN1_bm;
	PORTB.DIR |= PIN0_bm;
	
	USART3.BAUD = (uint16_t)USART4_BAUD_RATE(9600);
	USART3.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}
// ---------------------------------------------------------------
void USART3_sendChar(char c)
{
	while (!(USART3.STATUS & USART_DREIF_bm))
	{
		;
	}
	USART3.TXDATAL = c;	
}
// ---------------------------------------------------------------
void USART3_sendString(char *str)
{
	for(size_t i = 0; i < strlen(str); i++)
	{
		USART3_sendChar(str[i]);
	}
}
// ---------------------------------------------------------------
char USART3_readChar(bool echo)
{
char c;

	while (!(USART3.STATUS & USART_RXCIF_bm))
	{
		wdt_reset();
		;
	}
	c = USART3.RXDATAL;
	if ( echo)
		USART3_sendChar(c);
	return(c);
}
// ---------------------------------------------------------------
bool USART3_getChar( char *c )
{

	if ( USART3.STATUS & USART_RXCIF_bm) {
		*c = USART3.RXDATAL;
		return(true);
	}
	return(false);
}
// ---------------------------------------------------------------
void USART4_init(void)
{
	// Configuro para 115200
	
	PORTE.DIR &= ~PIN1_bm;
	PORTE.DIR |= PIN0_bm;
	
	USART4.BAUD = (uint16_t)USART4_BAUD_RATE(9600);
	USART4.CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}
// ---------------------------------------------------------------
void USART4_sendChar(char c)
{
	while (!(USART4.STATUS & USART_DREIF_bm))
	{
		;
	}
	USART4.TXDATAL = c;	
}
// ---------------------------------------------------------------
void USART4_sendString(char *str)
{
	for(size_t i = 0; i < strlen(str); i++)
	{
		USART4_sendChar(str[i]);
	}
}
// ---------------------------------------------------------------
char USART4_readChar(bool echo)
{
char c;

	while (!(USART4.STATUS & USART_RXCIF_bm))
	{
		wdt_reset();
		;
	}
	c = USART4.RXDATAL;
	if ( echo)
		USART4_sendChar(c);
	return(c);
}
// ---------------------------------------------------------------
bool USART4_getChar( char *c )
{

	if ( USART4.STATUS & USART_RXCIF_bm) {
		*c = USART4.RXDATAL;
		return(true);
	}
	return(false);
}
// ---------------------------------------------------------------
