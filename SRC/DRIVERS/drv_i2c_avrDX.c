
#include "drv_i2c_avrDX.h"


#define I2C_TIMEOUT 10000

#define TWI_BAUD(F_SCL, T_RISE) ((((((float)4000000 / (float)F_SCL)) - 10 - ((float)4000000 * T_RISE / 1000000))) / 2)


bool pv_i2c_send_Address_packet( volatile TWI_t *twi, uint8_t slaveAddress, uint8_t directionBit);
bool pv_i2c_send_Data_packet( volatile TWI_t *twi, uint8_t *dataBuffer, size_t length );
bool pv_i2c_rcvd_Data_packet( volatile TWI_t *twi, uint8_t *dataBuffer, size_t length );
bool pv_i2c_read_byte( volatile TWI_t *twi, uint8_t response_flag, char *rxByte );
bool pv_i2c_waitForComplete( volatile TWI_t *twi );
bool pv_i2c_set_bus_idle(volatile TWI_t *twi);

bool drv_i2c0_debug_flag, drv_i2c1_debug_flag;

//------------------------------------------------------------------------------
int drv_I2C_master_write ( volatile TWI_t *twi, const uint8_t devAddress, 
        const uint16_t dataAddress, 
        const uint8_t dataAddress_length, 
        char *pvBuffer, 
        size_t xBytes )
{

bool retV = false;
int xReturn = -1;
uint8_t packet[3];

    if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ){
		xprintf( "drv_I2C_master_write: START\r\n");
        xprintf( "  devAddr=%02X\r\n",devAddress);
        xprintf( "  dataAddr=%04X\r\n",dataAddress );
        xprintf( "  dataAddrLength=%d\r\n",dataAddress_length );
        xprintf( "  bytes2write=%d\r\n", xBytes );
	}

	// Fuerzo al bus al estado idle.
	// Paso 1: PONER EL BUS EN CONDICIONES
	if ( ! pv_i2c_set_bus_idle( twi ) ) {
		drv_I2C_reset( twi );
		goto i2c_quit;
	}

	// Pass1: Mando un START y el SLAVE_ADDRESS (SLA_W).
	if ( ! pv_i2c_send_Address_packet( twi, devAddress, I2C_DIRECTION_BIT_WRITE) ) {
		goto i2c_quit;
	}

	// Pass2: DATA_ADDRESS Mando la direccion interna del slave donde voy a escribir.
    if ( dataAddress_length == 2 ) {
        packet[0] = (dataAddress >> 8);         // HIGH_BYTE
        packet[1] = (dataAddress & 0x00FF );    // LOW_BYTE
    }  else {
        packet[0] = (dataAddress & 0x00FF );    // LOW_BYTE
    }
	if ( ! pv_i2c_send_Data_packet ( twi, packet, dataAddress_length ))  goto i2c_quit;

	// Pass3: Mando el buffer de datos. Debo recibir 0x28 (DATA_ACK) en c/u
	if ( ! pv_i2c_send_Data_packet ( twi, (uint8_t *)pvBuffer, xBytes ))  goto i2c_quit;
    
	xReturn = xBytes;
	retV = true;

i2c_quit:

	// Pass4) STOP
	twi->MCTRLB = TWI_MCMD_STOP_gc;

    if ( ( twi == &TWI0) && drv_i2c0_debug_flag ) {
        xprintf("STOP\r\n");
    }
    
    if ( ( twi == &TWI1) && drv_i2c1_debug_flag ) {
        xprintf("STOP\r\n");
    }

	// En caso de error libero la interface forzando el bus al estado IDLE
	if ( !retV )
		drv_I2C_reset( twi );

	return(xReturn);
}
//------------------------------------------------------------------------------
int drv_I2C_master_read ( volatile TWI_t *twi, const uint8_t devAddress,
					const uint16_t dataAddress,
					const uint8_t dataAddress_length,
					char *pvBuffer,
					size_t xBytes )
{
	// En el caso del ADC, el read no lleva la parte de mandar la SLA+W. !!!!!

bool retV = false;
int xReturn = -1;
uint8_t packet[3];

    if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ){
		xprintf( "drv_I2C_master_write: START\r\n");
        xprintf( "  devAddr=%02X\r\n",devAddress);
        xprintf( "  dataAddr=%04X\r\n",dataAddress );
        xprintf( "  dataAddrLength=%d\r\n",dataAddress_length );
        xprintf( "  bytes2read=%d\r\n", xBytes );
	}

	// Fuerzo al bus al estado idle.
	// Paso 1: PONER EL BUS EN CONDICIONES
	if ( ! pv_i2c_set_bus_idle( twi ) ) {
		drv_I2C_reset( twi );
		goto i2c_quit;
	}

	// Pass1: Mando un START y el SLAVE_ADDRESS (SLA_W).
	if ( ! pv_i2c_send_Address_packet( twi, devAddress, I2C_DIRECTION_BIT_WRITE) ) goto i2c_quit;

	// Pass2: DATA_ADDRESS Mando la direccion interna del slave donde voy a escribir.
    if ( dataAddress_length == 2 ) {
        packet[0] = (dataAddress >> 8);         // HIGH_BYTE
        packet[1] = (dataAddress & 0x00FF );    // LOW_BYTE
    }  else {
        packet[0] = (dataAddress & 0x00FF );    // LOW_BYTE
    }  
	if ( ! pv_i2c_send_Data_packet ( twi, packet, dataAddress_length ))  goto i2c_quit;

	// Pass3: Mando un START y el SLAVE_ADDRESS (SLA_W).
	if ( ! pv_i2c_send_Address_packet( twi, devAddress, I2C_DIRECTION_BIT_READ) ) goto i2c_quit;

	// Pass4: Leo todos los bytes requeridos y respondo a c/u con ACK.
	if ( ! pv_i2c_rcvd_Data_packet( twi, (uint8_t *)pvBuffer, xBytes )) goto i2c_quit;

    // Pass5) STOP
	twi->MCTRLB = TWI_MCMD_STOP_gc;
    
    if ( ( twi == &TWI0) && drv_i2c0_debug_flag ) {
        xprintf("STOP\r\n");
    }
    
    if ( ( twi == &TWI1) && drv_i2c1_debug_flag ) {
        xprintf("STOP\r\n");
    }
    
	xReturn = xBytes;
	retV = true;

i2c_quit:

	// En caso de error libero la interface forzando el bus al estado IDLE
	if ( !retV )
		drv_I2C_reset( twi );

	return(xReturn);

}
//------------------------------------------------------------------------------
void drv_I2C_init( volatile TWI_t *twi )
{
    if ( twi == &TWI0) {
        drv_i2c0_debug_flag = false;
    }
    
	PF3_set_level(false);
	PF3_set_dir(PORT_DIR_OUT);
	PF3_set_pull_mode(PORT_PULL_OFF);
	PF3_set_inverted(false);
	PF3_set_isc(PORT_ISC_INTDISABLE_gc);

	PF2_set_level(false);
	PF2_set_dir(PORT_DIR_OUT);
	PF2_set_pull_mode(PORT_PULL_OFF);
	PF2_set_inverted(false);
	PF2_set_isc(PORT_ISC_INTDISABLE_gc);

	// set i2c bit rate to 100KHz
	twi->MBAUD = (uint8_t)TWI_BAUD(100000, 0);
	
	// enable TWI (two-wire interface)
	twi->MCTRLA = 1 << TWI_ENABLE_bp        /* Enable TWI Master: enabled */
	| 0 << TWI_QCEN_bp						/* Quick Command Enable: disabled */
	| 0 << TWI_RIEN_bp						/* Read Interrupt Enable: disabled */
	| 0 << TWI_SMEN_bp						/* Smart Mode Enable: disabled */
	| TWI_TIMEOUT_DISABLED_gc				/* Bus Timeout Disabled */
	| 0 << TWI_WIEN_bp;						/* Write Interrupt Enable: disabled */
}
//------------------------------------------------------------------------------
void drv_I2C_config_debug( volatile TWI_t *twi, bool debug_flag)
{
    if ( twi == &TWI0) {
        drv_i2c0_debug_flag = debug_flag;
        return;
    }

    if ( twi == &TWI1) {
        drv_i2c1_debug_flag = debug_flag;
        return;
    }
    
}
//------------------------------------------------------------------------------
void drv_I2C_reset( volatile TWI_t *twi )
{
    /* 
     * https://stackoverflow.com/questions/5497488/failed-twi-transaction-after-sleep-on-xmega
     * There is a common problem on I2C/TWI where the internal state machine gets stuck in an
     * intermediate state if a transaction is not completed fully. The slave then does not respond
     * correctly when addressed on the next transaction. This commonly happens when the master
     * is reset or stops outputting the SCK signal part way through the read or write.
     * A solution is to toggle the SCK line manually 8 or 9 times before starting any data
     * transactions so the that the internal state machines in the slaves are all reset to the
     * start of transfer point and they are all then looking for their address byte
     */
    
    vTaskDelay( ( TickType_t)( 10 /  portTICK_PERIOD_MS ) );	
	twi->MCTRLB |= TWI_FLUSH_bm;
    twi->MSTATUS |= TWI_BUSSTATE_IDLE_gc;	// Pongo el status en 01 ( idle )
	// Reset module
	twi->MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);  
}
//------------------------------------------------------------------------------
// FUNCIONES PRIVADAS
//------------------------------------------------------------------------------
bool pv_i2c_send_Address_packet(volatile TWI_t *twi, uint8_t slaveAddress, uint8_t directionBit)
{

	// Pass1: Mando un START y el SLAVE_ADDRESS (SLA_W/R)
	// El start se genera automaticamente al escribir en el reg MASTER.ADDR.
	// Esto tambien resetea todas las flags.
	// La salida correcta es con STATUS = 0x62.
	// El escribir el ADDR borra todas las flags.

char txbyte;	// (SLA_W/R) Send slave address
bool ret_code = false;

	if ( directionBit == I2C_DIRECTION_BIT_WRITE ) {
		txbyte = slaveAddress & ~0x01;
	} else {
		txbyte = slaveAddress | 0x01;
	}
    // Esto genera un START y se envia el devAddress
	twi->MADDR = txbyte;
	if ( ! pv_i2c_waitForComplete( twi ) ) goto i2c_exit;

	// Primero evaluo no tener errores.
	if ( (twi->MSTATUS & TWI_ARBLOST_bm) != 0 ) goto i2c_exit;
	if ( (twi->MSTATUS & TWI_BUSERR_bm) != 0 ) goto i2c_exit;

	// ACK o NACK ?
	if ( (twi->MSTATUS & TWI_RXACK_bm) != 0 ) {
		// NACK
        xprintf("pvI2C_write_slave_address: NACK status=0x%02x\r\n", twi->MSTATUS );
		goto i2c_exit;
	} else {
		// ACK
		ret_code = true;
		goto i2c_exit;
	}

i2c_exit:

    // DEBUG
    if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ) {
        xprintf("ADDR=0x%02x, ST=0x%02x\r\n", txbyte, twi->MSTATUS );
    }

	return(ret_code);

}
//------------------------------------------------------------------------------
bool pv_i2c_send_Data_packet( volatile TWI_t *twi, uint8_t *dataBuffer, size_t length )
{

uint8_t bytesWritten;
uint8_t txbyte;
bool retS = false;
    
    // DEBUG
    if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ) {
        xprintf_P(PSTR("SEND DATAPKT:"));
    }

	// No hay datos para enviar: dummy write.
	if ( length == 0 )
		return(true);

	// Mando el buffer de datos. Debo recibir 0x28 (DATA_ACK) en c/u
	for ( bytesWritten=0; bytesWritten < length; bytesWritten++ ) {
		txbyte = *dataBuffer++;
		twi->MDATA = txbyte;		// send byte
        
        // DEBUG
        if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ) {
            if ( (bytesWritten % 8) == 0 ) {
                xprintf_P(PSTR("\r\n%02d: "), bytesWritten);
            }
            
            xprintf("[0x%02x,ST=0x%02x] ", txbyte, twi->MSTATUS );
        }

        
		if ( ! pv_i2c_waitForComplete(twi) ) {
            xprintf("pv_I2C_send_data: ERROR (status=%d)\r\n", twi->MSTATUS );
            goto i2c_exit;
        }

		// NACK ?
		if ( (twi->MSTATUS & TWI_RXACK_bm) != 0 )  {
            xprintf("pv_I2C_send_data: NACK (status=%d)\r\n", twi->MSTATUS );
            goto i2c_exit;
        }
        
	}

	// Envie todo el buffer
    retS = true;

i2c_exit:

    // DEBUG
    if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ) {
         xprintf_P(PSTR("\r\n"));   
    }
	return(retS);

}
//------------------------------------------------------------------------------
bool pv_i2c_rcvd_Data_packet( volatile TWI_t *twi, uint8_t *dataBuffer, size_t length )
{

char rxByte;
bool retS = false;
uint8_t i;

    // DEBUG
    if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ) {
        xprintf_P(PSTR("RCVD DATAPKT:"));
    }

    i = 0;
	while(length > 1) {
		if ( ! pv_i2c_read_byte( twi, ACK, &rxByte) ) goto i2c_quit;
		*dataBuffer++ = rxByte;
		// decrement length
		length--;
        
        // DEBUG
        if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ) {
            if ( (i % 8) == 0 ) {
                xprintf_P(PSTR("\r\n%02d: "), i);
            }
            xprintf("[0x%02x,ST=0x%02x] ", rxByte, twi->MSTATUS );
        }
        i++;
	}

	// accept receive data and nack it (last-byte signal)
	// Ultimo byte.
	if ( ! pv_i2c_read_byte( twi, NACK, &rxByte) ) goto i2c_quit;
	*dataBuffer++ = rxByte;
    // DEBUG
    if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ) {
        xprintf("[0x%02x,ST=0x%02x]", rxByte, twi->MSTATUS );
    }

	retS = true;

i2c_quit:

    // DEBUG
    if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ) {
         xprintf_P(PSTR("\r\n"));
    }

	return(retS);
}
//------------------------------------------------------------------------------
bool pv_i2c_read_byte( volatile TWI_t *twi, uint8_t response_flag, char *rxByte )
{

bool ret_code = false;
uint8_t currentStatus = 0;
	
	// Espero 1 byte enviado por el slave
	if ( ! pv_i2c_waitForComplete( twi ) ) {
        xprintf(" pvI2C_read_byte TO: st=0x%02x\r\n", twi->MSTATUS );
        goto i2c_exit;
    }
	currentStatus = twi->MSTATUS;

	*rxByte = twi->MDATA;

 //   if ( (( twi == &TWI0) && drv_i2c0_debug_flag ) || (( twi == &TWI1) && drv_i2c1_debug_flag ) ) {
 //       xprintf("[RB=0x%02x, ST=0x%02x] ",*rxByte,currentStatus );
 //   }
    
	if (response_flag == ACK) {
        twi->MCTRLB |= TWI_ACKACT_ACK_gc | TWI_MCMD_RECVTRANS_gc; // ACK y Mas bytes     
	} else {
		twi->MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;	 // NACK Ultimo byte + STOP
	}

	// Primero evaluo no tener errores.
	if ( (currentStatus & TWI_ARBLOST_bm) != 0 ) {
        xprintf("pvI2C_read_byte ARBLOST: 0x%02x, st=0x%02x\r\n",*rxByte,currentStatus );
        goto i2c_exit;
    }
	if ( (currentStatus & TWI_BUSERR_bm) != 0 ) {
        xprintf("pvI2C_read_byte BUSERR: 0x%02x, st=0x%02x\r\n",*rxByte,currentStatus );
        goto i2c_exit;
    }

	ret_code = true;
   

i2c_exit:

	return(ret_code);
}
//------------------------------------------------------------------------------
bool pv_i2c_waitForComplete( volatile TWI_t *twi )
{

uint8_t ticks_to_wait = 30;		// 3 ticks ( 30ms es el maximo tiempo que espero )
bool retS = false;

	// wait for i2c interface to complete write operation ( MASTER WRITE INTERRUPT )
	while ( ticks_to_wait-- > 0 ) {
		if ( ( ( twi->MSTATUS & TWI_WIF_bm) != 0 ) || ( ( twi->MSTATUS & TWI_RIF_bm) != 0 ) ) {
			retS = true;
			goto quit;
		}
		vTaskDelay( ( TickType_t)( 5 ) );
	}

	retS = false;

quit:

	return(retS);

}
//------------------------------------------------------------------------------
bool pv_i2c_set_bus_idle( volatile TWI_t *twi)
{
    // Intenta hasta 3 veces poner el bus en estado idle
	// Para comenzar una operacion el bus debe estar en IDLE o OWENED.
	// Intento pasarlo a IDLE hasta 3 veces antes de abortar, esperando 100ms
	// entre c/intento.

uint8_t	reintentos = I2C_MAXTRIES;

	while ( reintentos-- > 0 ) {

		// Los bits CLKHOLD y RXACK son solo de read por eso la mascara !!!
		if (  ( ( twi->MSTATUS & TWI_BUSSTATE_gm ) == TWI_BUSSTATE_IDLE_gc ) ||
				( ( twi->MSTATUS & TWI_BUSSTATE_gm ) == TWI_BUSSTATE_OWNER_gc ) ) {
			return(true);

		} else {
			// El status esta indicando errores. Debo limpiarlos antes de usar la interface.
			if ( (twi->MSTATUS & TWI_ARBLOST_bm) != 0 ) {
				twi->MSTATUS |= TWI_ARBLOST_bm;
			}
			if ( (twi->MSTATUS & TWI_BUSERR_bm) != 0 ) {
				twi->MSTATUS |= TWI_BUSERR_bm;
			}
			if ( (twi->MSTATUS & TWI_WIF_bm) != 0 ) {
				twi->MSTATUS |= TWI_WIF_bm;
			}
			if ( (twi->MSTATUS & TWI_RIF_bm) != 0 ) {
				twi->MSTATUS |= TWI_RIF_bm;
			}

			twi->MSTATUS |= TWI_BUSSTATE_IDLE_gc;	// Pongo el status en 01 ( idle )
			vTaskDelay( ( TickType_t)( 10 /  portTICK_PERIOD_MS ) );
		}
	}

	// No pude pasarlo a IDLE: Error !!!
    xprintf("pv_i2c_set_bus_idle ERROR!!: status=0x%02x\r\n\0", twi->MSTATUS );

	return(false);
}
//------------------------------------------------------------------------------
