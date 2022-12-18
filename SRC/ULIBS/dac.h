/* 
 * File:   dac.h
 * Author: pablo
 *
 * Created on 10 de febrero de 2022, 12:32 PM
 */

#ifndef DAC_H
#define	DAC_H

#ifdef	__cplusplus
extern "C" {
#endif


//#include "xc.h"
#include <avr/io.h>
#include "stdint.h"
    
/* Mask needed to get the 2 LSb for DAC Data Register */
#define LSB_MASK    (0x03)
        
void VREF_init(void);
void DAC_init(void);
void DAC_setVal(uint16_t value);


#ifdef	__cplusplus
}
#endif

#endif	/* DAC_H */

