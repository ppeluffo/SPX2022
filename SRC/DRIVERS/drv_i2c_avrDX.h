/* 
 * File:   drv_i2c_avrDX.h
 * Author: pablo
 *
 * Created on 15 de marzo de 2022, 08:50 AM
 */

#ifndef DRV_I2C_AVRDX_H
#define	DRV_I2C_AVRDX_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdio.h" 
#include "xprintf.h"
#include "atmel_start_pins.h"
#include "clock_config.h"
//#include <util/delay.h>
 #include <stdbool.h>
#include <util/twi.h>
#include "FreeRTOS.h"
#include "task.h"  

#define I2C_DIRECTION_BIT_WRITE 0
#define I2C_DIRECTION_BIT_READ  1
#define I2C_MAXTRIES	5

#define ACK 	0
#define NACK 	1

/*! Transaction result enumeration. */
typedef enum TWIM_RESULT_enum {
	TWIM_RESULT_UNKNOWN          = (0x00<<0),
	TWIM_RESULT_OK               = (0x01<<0),
	TWIM_RESULT_BUFFER_OVERFLOW  = (0x02<<0),
	TWIM_RESULT_ARBITRATION_LOST = (0x03<<0),
	TWIM_RESULT_BUS_ERROR        = (0x04<<0),
	TWIM_RESULT_NACK_RECEIVED    = (0x05<<0),
	TWIM_RESULT_FAIL             = (0x06<<0),
	TWIM_RESULT_TIMEOUT          = (0x07<<0),
} TWIM_RESULT_t;

void drv_I2C_init( volatile TWI_t *twi );
void drv_I2C_config_debug( volatile TWI_t *twi, bool debug_flag);
void drv_I2C_reset( volatile TWI_t *twi );
int  drv_I2C_master_write ( volatile TWI_t *twi, const uint8_t devAddress, const uint16_t dataAddress, const uint8_t dataAddress_length, char *pvBuffer, size_t xBytes );
int  drv_I2C_master_read  ( volatile TWI_t *twi, const uint8_t devAddress, const uint16_t dataAddress, const uint8_t dataAddress_length, char *pvBuffer, size_t xBytes );


#ifdef	__cplusplus
}
#endif

#endif	/* DRV_I2C_AVRDX_H */

