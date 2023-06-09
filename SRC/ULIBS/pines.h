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

#define CONFIG_RTS_485B()       RTS_RS485B_PORT.DIR |= RTS_RS485B_PIN_bm

// DRV8814:
#define DRV8814_RESET_PORT      PORTA      
#define DRV8814_RESET           6
#define DRV8814_RESET_PIN_bm    PIN6_bm
#define DRV8814_RESET_PIN_bp    PIN6_bp
#define SET_DRV8814_RESET       ( DRV8814_RESET_PORT.OUT |= DRV8814_RESET_PIN_bm )
#define CLEAR_DRV8814_RESET     ( DRV8814_RESET_PORT.OUT &= ~DRV8814_RESET_PIN_bm )
#define CONFIG_DRV8814_RESET    ( DRV8814_RESET_PORT.DIR |= DRV8814_RESET_PIN_bm )

#define DRV8814_SLEEP_PORT      PORTA     
#define DRV8814_SLEEP           7
#define DRV8814_SLEEP_PIN_bm    PIN7_bm
#define DRV8814_SLEEP_PIN_bp    PIN7_bp
#define SET_DRV8814_SLEEP       ( DRV8814_SLEEP_PORT.OUT |= DRV8814_SLEEP_PIN_bm )
#define CLEAR_DRV8814_SLEEP     ( DRV8814_SLEEP_PORT.OUT &= ~DRV8814_SLEEP_PIN_bm )
#define CONFIG_DRV8814_SLEEP    ( DRV8814_SLEEP_PORT.DIR |= DRV8814_SLEEP_PIN_bm )

#define DRV8814_AEN_PORT      PORTB      
#define DRV8814_AEN           4
#define DRV8814_AEN_PIN_bm    PIN4_bm
#define DRV8814_AEN_PIN_bp    PIN4_bp
#define SET_DRV8814_AEN       ( DRV8814_AEN_PORT.OUT |= DRV8814_AEN_PIN_bm )
#define CLEAR_DRV8814_AEN     ( DRV8814_AEN_PORT.OUT &= ~DRV8814_AEN_PIN_bm )
#define CONFIG_DRV8814_AEN    ( DRV8814_AEN_PORT.DIR |= DRV8814_AEN_PIN_bm )

#define DRV8814_BEN_PORT      PORTB      
#define DRV8814_BEN           5
#define DRV8814_BEN_PIN_bm    PIN5_bm
#define DRV8814_BEN_PIN_bp    PIN5_bp
#define SET_DRV8814_BEN       ( DRV8814_BEN_PORT.OUT |= DRV8814_BEN_PIN_bm )
#define CLEAR_DRV8814_BEN     ( DRV8814_BEN_PORT.OUT &= ~DRV8814_BEN_PIN_bm )
#define CONFIG_DRV8814_BEN    ( DRV8814_BEN_PORT.DIR |= DRV8814_BEN_PIN_bm )

#define DRV8814_APH_PORT      PORTB      
#define DRV8814_APH           0
#define DRV8814_APH_PIN_bm    PIN0_bm
#define DRV8814_APH_PIN_bp    PIN0_bp
#define SET_DRV8814_APH       ( DRV8814_APH_PORT.OUT |= DRV8814_APH_PIN_bm )
#define CLEAR_DRV8814_APH     ( DRV8814_APH_PORT.OUT &= ~DRV8814_APH_PIN_bm )
#define CONFIG_DRV8814_APH    ( DRV8814_APH_PORT.DIR |= DRV8814_APH_PIN_bm )

#define DRV8814_BPH_PORT      PORTB      
#define DRV8814_BPH           1
#define DRV8814_BPH_PIN_bm    PIN1_bm
#define DRV8814_BPH_PIN_bp    PIN1_bp
#define SET_DRV8814_BPH       ( DRV8814_BPH_PORT.OUT |= DRV8814_BPH_PIN_bm )
#define CLEAR_DRV8814_BPH     ( DRV8814_BPH_PORT.OUT &= ~DRV8814_BPH_PIN_bm )
#define CONFIG_DRV8814_BPH    ( DRV8814_BPH_PORT.DIR |= DRV8814_BPH_PIN_bm )

// Los pines de FinCarrera son entradas
#define FC1_PORT      PORTB    
#define FC1           7
#define FC1_PIN_bm    PIN7_bm
#define FC1_PIN_bp    PIN7_bp
#define CONFIG_FC1    ( FC1_PORT.DIR &= ~FC1_PIN_bm )

#define FC2_PORT      PORTB     
#define FC2           6
#define FC2_PIN_bm    PIN6_bm
#define FC2_PIN_bp    PIN6_bp
#define CONFIG_FC2    ( FC2_PORT.DIR &= ~FC2_PIN_bm )


#ifdef	__cplusplus
}
#endif

#endif	/* PINES_H */

