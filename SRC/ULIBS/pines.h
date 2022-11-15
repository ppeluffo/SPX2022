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

#define SET_RELEOUT()      CLEAR_OCOUT() 
#define CLEAR_RELEOUT()    SET_OCOUT()
#define TOGGLE_RELEOUT()   TOGGLE_OCOUT()
    
void OCOUT_init(void);
    

// Salida Control de VSENSOR.
       
#define VSENSOR_PORT         PORTD
#define VSENSOR              1
#define VSENSOR_PIN_bm       PIN1_bm
#define VSENSOR_PIN_bp       PIN1_bp

#define SET_VSENSOR()       ( VSENSOR_PORT.OUT |= VSENSOR_PIN_bm )
#define CLEAR_VSENSOR()     ( VSENSOR_PORT.OUT &= ~VSENSOR_PIN_bm )
    
void VSENSOR_init(void);


// Salida de prender/apagar sensores 4-20
#define SENSORS420_PORT         PORTD
#define SENSORS420              1
#define SENSORS420_PIN_bm       PIN1_bm
#define SENSORS420_PIN_bp       PIN1_bp

#define SET_SENSORS420()       ( SENSORS420_PORT.OUT |= SENSORS420_PIN_bm )
#define CLEAR_SENSORS420()     ( SENSORS420_PORT.OUT &= ~SENSORS420_PIN_bm )

void SENSORS420_init(void);

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

