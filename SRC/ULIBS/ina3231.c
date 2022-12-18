/*
 * ina3232.c
 *
 *  Created on: 8 dic. 2018
 *      Author: pablo
 */

#include "ina3221.h"

//------------------------------------------------------------------------------
void INA_config( uint16_t conf_reg_value )
{
char res[3] = { 0 };
int16_t xBytes = 0;

	res[0] = ( conf_reg_value & 0xFF00 ) >> 8;
	res[1] = ( conf_reg_value & 0x00FF );

	xBytes = INA_write( INA3231_CONF, res, 2 );
	if ( xBytes == -1 )
		xprintf("ERROR: I2C:INA_config\r\n");

}
//------------------------------------------------------------------------------
int16_t INA_test_write ( char *rconf_val_str )
{

	// Escribe en el registro de configuracion de un INA ( 0, 1, 2)

uint16_t val = 0;
char data[3] = { 0 };
int16_t xBytes = 0;

	val = atoi( rconf_val_str);
	data[0] = ( val & 0xFF00 ) >> 8;
	data[1] = ( val & 0x00FF );
	xBytes = INA_write( INA3231_CONF, data, 2 );
	if ( xBytes == -1 )
		xprintf("ERROR: I2C:INA_test_write\r\n");

	return (xBytes);
}
//------------------------------------------------------------------------------
int16_t INA_test_read ( char *regs )
{

uint16_t val = 0;
char data[3] = { ' ' };
int16_t xBytes = 0;
char l_data[10] = { ' ' };

	memcpy(l_data, regs, sizeof(l_data));
	strupr(l_data);

    // Awake
    //INA_config_avg128(INA_A);
    
	// read ina id {conf|chxshv|chxbusv|mfid|dieid}
	if (strcmp( l_data, "CONF") == 0 ) {
		xBytes = INA_read( INA3231_CONF, data, 2 );
	} else if (strcmp( l_data, "CH1SHV") == 0) {
		xBytes = INA_read( INA3221_CH1_SHV, data, 2 );
	} else if (strcmp( l_data,"CH1BUSV\0") == 0) {
		xBytes = INA_read( INA3221_CH1_BUSV, data, 2 );
	} else if (strcmp( l_data, "CH2SHV\0") == 0) {
		xBytes = INA_read( INA3221_CH2_SHV, data, 2 );
	} else if (strcmp( l_data, "CH2BUSV\0") == 0) {
		xBytes = INA_read( INA3221_CH2_BUSV, data, 2 );
	} else if (strcmp( l_data, "CH3SHV\0") == 0) {
		xBytes = INA_read( INA3221_CH3_SHV, data, 2 );
	} else if (strcmp( l_data, "CH3BUSV\0") == 0) {
		xBytes = INA_read( INA3221_CH3_BUSV, data, 2 );
	} else if (strcmp( l_data, "MFID\0") == 0) {
		xBytes = INA_read( INA3221_MFID, data, 2 );
	} else if (strcmp( l_data, "DIEID\0") == 0) {
		xBytes = INA_read( INA3221_DIEID, data, 2 );
	} else {
		xBytes = -1;
	}

	if ( xBytes == -1 ) {
		xprintf("ERROR: I2C:INA_test_read\r\n\0");

	} else {

		val = ( data[0]<< 8 ) + data[1];
		xprintf("INA_VAL=0x%04x\r\n\0", val);
	}

    // Sleep
    //INA_config_sleep(INA_A);
    
	return(xBytes);

}
//------------------------------------------------------------------------------
int16_t INA_read( uint16_t rdAddress, char *data, uint8_t length )
{

int16_t rcode = 0;

	rcode =  I2C_read( fdI2C1, DEVADDRESS_INA, rdAddress, 0x01, data, length );
	if ( rcode == -1 ) {
		// Hubo error: trato de reparar el bus y reintentar la operacion
		// Espero 1s que se termine la fuente de ruido.
		vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
		// Reconfiguro los dispositivos I2C del bus que pueden haberse afectado
		xprintf("ERROR: INA_read recovering i2c bus\r\n" );
		goto quit;
	}

quit:

	return( rcode );

}
//------------------------------------------------------------------------------
int16_t INA_write( uint16_t wrAddress, char *data, uint8_t length )
{

int16_t rcode = 0;

	rcode =  I2C_write ( fdI2C1, DEVADDRESS_INA, wrAddress, 0x01, data, length );
	if ( rcode == -1 ) {
		// Hubo error: trato de reparar el bus y reintentar la operacion
		// Espero 1s que se termine la fuente de ruido.
		vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );
		// Reconfiguro los dispositivos I2C del bus que pueden haberse afectado
		xprintf("ERROR: INA_write recovering i2c bus\r\n" );
	}

	return( rcode );

}
//------------------------------------------------------------------------------
