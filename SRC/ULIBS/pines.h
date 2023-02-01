/* 
 * File:   pines.h
 * Author: pablo
 *
 * Created on 11 de febrero de 2022, 06:02 PM
 */

#ifndef PINES_H
#define	PINES_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <avr/io.h>
#include "stdbool.h"
    
//--------------------------------------------------------------------------
// Salida OC.
       
#define OCOUT_PORT         PORTD
#define OCOUT              7 
#define OCOUT_PIN_bm       PIN7_bm
#define OCOUT_PIN_bp       PIN7_bp

#define SET_OCOUT()       ( OCOUT_PORT.OUT |= OCOUT_PIN_bm )
#define CLEAR_OCOUT()     ( OCOUT_PORT.OUT &= ~OCOUT_PIN_bm )
#define TOGGLE_OCOUT()    ( OCOUT_PORT.OUT ^= 1UL << OCOUT_PIN_bp);

#define OCOUT_OPEN()         CLEAR_OCOUT() 
#define OCOUT_CLOSE()        SET_OCOUT()

void OCOUT_init(void);

#define RELE_K1_PORT         PORTA
#define RELE_K1              5 
#define RELE_K1_PIN_bm       PIN5_bm
#define RELE_K1_PIN_bp       PIN5_bp

#define SET_RELE_K1()       ( RELE_K1_PORT.OUT |= RELE_K1_PIN_bm )
#define CLEAR_RELE_K1()     ( RELE_K1_PORT.OUT &= ~RELE_K1_PIN_bm )
#define TOGGLE_RELE_K1()    ( RELE_K1_PORT.OUT ^= 1UL << RELE_K1_PIN_bp);
    
#define RELE_K1_OPEN()      CLEAR_RELE_K1() 
#define RELE_K1_CLOSE()     SET_RELE_K1()

void RELE_K1_init(void);
    
#define RELE_K2_PORT         PORTA
#define RELE_K2              6 
#define RELE_K2_PIN_bm       PIN6_bm
#define RELE_K2_PIN_bp       PIN6_bp

#define SET_RELE_K2()       ( RELE_K2_PORT.OUT |= RELE_K2_PIN_bm )
#define CLEAR_RELE_K2()     ( RELE_K2_PORT.OUT &= ~RELE_K2_PIN_bm )
#define TOGGLE_RELE_K2()    ( RELE_K2_PORT.OUT ^= 1UL << RELE_K2_PIN_bp);
    
#define RELE_K2_OPEN()      CLEAR_RELE_K2() 
#define RELE_K2_CLOSE()     SET_RELE_K2()

void RELE_K2_init(void);

// Salida de prender/apagar sensores 4-20
#define VSENSORS420_PORT         PORTD
#define VSENSORS420              1
#define VSENSORS420_PIN_bm       PIN1_bm
#define VSENSORS420_PIN_bp       PIN1_bp

#define SET_VSENSORS420()       ( VSENSORS420_PORT.OUT |= VSENSORS420_PIN_bm )
#define CLEAR_VSENSORS420()     ( VSENSORS420_PORT.OUT &= ~VSENSORS420_PIN_bm )

void VSENSORS420_init(void);

#define RTS_RS485A_PORT         PORTC
#define RTS_RS485A              2
#define RTS_RS485A_PIN_bm       PIN2_bm
#define RTS_RS485A_PIN_bp       PIN2_bp
#define SET_RTS_RS485A()        ( RTS_RS485A_PORT.OUT |= RTS_RS485A_PIN_bm )
#define CLEAR_RTS_RS485A()      ( RTS_RS485A_PORT.OUT &= ~RTS_RS485A_PIN_bm )

#define CONFIG_RTS_485A()       RTS_RS485A_PORT.DIR |= RTS_RS485A_PIN_bm;


#define RTS_RS485B_PORT         PORTG
#define RTS_RS485B              7
#define RTS_RS485B_PIN_bm       PIN7_bm
#define RTS_RS485B_PIN_bp       PIN7_bp
#define SET_RTS_RS485B()        ( RTS_RS485B_PORT.OUT |= RTS_RS485B_PIN_bm )
#define CLEAR_RTS_RS485B()      ( RTS_RS485B_PORT.OUT &= ~RTS_RS485B_PIN_bm )

#define CONFIG_RTS_485B()       RTS_RS485B_PORT.DIR |= RTS_RS485B_PIN_bm;


#ifdef	__cplusplus
}
#endif

#endif	/* PINES_H */

