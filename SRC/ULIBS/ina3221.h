/*
 * l_ina3223.h
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#ifndef SRC_SPX_LIBS_L_INA3221_H_
#define SRC_SPX_LIBS_L_INA3221_H_

#include "frtos-io.h"
#include "stdint.h"
#include "i2c.h"
#include "xprintf.h"

//------------------------------------------------------------------------------------

// WORDS de configuracion de los INAs
#define CONF_INA_SLEEP			0x7920
#define CONF_INA_AVG128         0x7927
#define CONF_INA_PWRDOWN		0x0000


// Direcciones de los registros de los INA
#define INA3231_CONF			0x00
#define INA3221_CH1_SHV			0x01
#define INA3221_CH1_BUSV		0x02
#define INA3221_CH2_SHV			0x03
#define INA3221_CH2_BUSV		0x04
#define INA3221_CH3_SHV			0x05
#define INA3221_CH3_BUSV		0x06
#define INA3221_MFID			0xFE
#define INA3221_DIEID			0xFF

#define INA3221_VCC_SETTLE_TIME	500

#define INA_awake() INA_config(CONF_INA_AVG128)
#define INA_sleep() INA_config(CONF_INA_SLEEP)

//------------------------------------------------------------------------------------
// API publica
void INA_config( uint16_t conf_reg_value );
//
int16_t INA_read( uint16_t rdAddress, char *data, uint8_t length );
int16_t INA_write( uint16_t wrAddress, char *data, uint8_t length );
//
int16_t INA_test_write ( char *rconf_val_str );
int16_t INA_test_read ( char *regs );
//
// API END
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------

#endif /* SRC_SPX_LIBS_L_INA3221_H_ */
