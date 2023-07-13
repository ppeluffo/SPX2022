


#include "modbus.h"
#include "math.h"


static bool f_debug_modbus;

uint16_t pv_modbus_CRC16( uint8_t *msg, uint8_t msg_size );

void pv_decoder_f3_c0123( mbus_CONTROL_BLOCK_t *mbus_cb, uint8_t payload_ptr );
void pv_decoder_f3_c3210( mbus_CONTROL_BLOCK_t *mbus_cb, uint8_t payload_ptr );
void pv_decoder_f3_c1032( mbus_CONTROL_BLOCK_t *mbus_cb, uint8_t payload_ptr );
void pv_decoder_f3_c2301( mbus_CONTROL_BLOCK_t *mbus_cb, uint8_t payload_ptr );

void pv_encoder_f6_c0123(mbus_CONTROL_BLOCK_t *mbus_cb );
void pv_encoder_f6_c2301(mbus_CONTROL_BLOCK_t *mbus_cb );
void pv_encoder_f6_c3210(mbus_CONTROL_BLOCK_t *mbus_cb );
void pv_encoder_f6_c1032(mbus_CONTROL_BLOCK_t *mbus_cb );

void pv_encoder_f16_c0123(mbus_CONTROL_BLOCK_t *mbus_cb );
void pv_encoder_f16_c2301(mbus_CONTROL_BLOCK_t *mbus_cb );
void pv_encoder_f16_c3210(mbus_CONTROL_BLOCK_t *mbus_cb );
void pv_encoder_f16_c1032(mbus_CONTROL_BLOCK_t *mbus_cb );

void pv_modbus_make_ADU( mbus_CONTROL_BLOCK_t *mbus_cb );
void pv_modbus_txmit_ADU( mbus_CONTROL_BLOCK_t *mbus_cb );

mbus_CONTROL_BLOCK_t mbus_cb;

int FD_MODBUS;
// Puntero al inicio del buffer de modbus ( para copiarlo )
int MODBUS_BUFFER_SIZE;
// Puntero a la funcion que borra el buffer de recepcion de modbus.
void (*ptrFuncFlush) (void); 
// Puntero a la funcion que nos da cuantos bytes hay en el buffer de lectura
uint16_t (*ptrFuncGetCount) (void);
// Puntero que nos da la direccion de comienzo del buffer de recepcion 
char *(*ptrFuncRXBufferInit) (void);

modbus_conf_t modbus_conf;

// -----------------------------------------------------------------------------
void modbus_update_local_config( modbus_conf_t *modbus_system_conf)
{
    memcpy( modbus_system_conf, &modbus_conf, sizeof(modbus_conf_t));
}
// -----------------------------------------------------------------------------
void modbus_read_local_config( modbus_conf_t *modbus_system_conf)
{
    memcpy( &modbus_conf, modbus_system_conf, sizeof(modbus_conf_t)); 
}
//------------------------------------------------------------------------------
void modbus_init( int fd_modbus, int buffer_size, void (*f)(void), uint16_t (*g)(void), char *(*h)(void)  )
{
    /*
     * Fijo cual es el fd por donde vamos a hacer las transmisiones
     * y cual funcion voy a usar para leer los datos recibidos.
     */
    
    FD_MODBUS = fd_modbus;
    MODBUS_BUFFER_SIZE = buffer_size;
    
    ptrFuncFlush = f;
    ptrFuncGetCount = g;
    ptrFuncRXBufferInit = h;
    
}
//------------------------------------------------------------------------------
void modbus_config_debug(bool debug )
{
    if ( debug ) {
        f_debug_modbus = true;
    } else {
        f_debug_modbus = false;
    }
    
}
//------------------------------------------------------------------------------
bool modbus_read_debug(void)
{
    return (f_debug_modbus);
}
//------------------------------------------------------------------------------
void modbus_config_defaults(void)
{

uint8_t i;

    modbus_conf.enabled = false;
    modbus_conf.localaddr = 0x01;
	for ( i = 0; i < NRO_MODBUS_CHANNELS; i++ ) {
        modbus_conf.mbch[i].enabled = false;
        snprintf_P( modbus_conf.mbch[i].name, MODBUS_PARAMNAME_LENGTH, PSTR("X"));
		modbus_conf.mbch[i].slave_address = 0;
		modbus_conf.mbch[i].reg_address = 0;
        modbus_conf.mbch[i].nro_regs = 1;
        modbus_conf.mbch[i].fcode = 3;
		modbus_conf.mbch[i].type = u16;
		modbus_conf.mbch[i].codec = CODEC0123;
		modbus_conf.mbch[i].divisor_p10 = 0;
	};
}
//------------------------------------------------------------------------------
void modbus_print_configuration()
{
uint8_t i = 0;

    xprintf_P( PSTR("Modbus: ( name,sla_addr,reg_addr,nro_regs,rcode,type,codec,factor_p10 )\r\n"));
    xprintf_P(PSTR(" debug: "));
    f_debug_modbus ? xprintf_P(PSTR("true\r\n")) : xprintf_P(PSTR("false\r\n"));

    if ( modbus_conf.enabled ) {
        xprintf_P(PSTR(" status=enabled\r\n"));
    } else {
         xprintf_P(PSTR(" status=disabled\r\n"));
    }
    
    // Si esta deshabilitado no tengo porque mostrar los canales.
    if ( ! modbus_conf.enabled ) {
        return;
    }
    
    xprintf_P(PSTR(" localaddr:0x%02x\r\n"), modbus_conf.localaddr);
    
    
	for ( i = 0; i < NRO_MODBUS_CHANNELS; i++) {
 
        if ( modbus_conf.mbch[i].enabled ) {
            xprintf_P( PSTR(" M%02d: +"),i);
        } else {
            xprintf_P( PSTR(" M%02d: -"),i);
        }
                
        xprintf_P( PSTR("[%s,%d,%d,%d,%d,"),
            modbus_conf.mbch[i].name,
            modbus_conf.mbch[i].slave_address,
            modbus_conf.mbch[i].reg_address,
            modbus_conf.mbch[i].nro_regs, 
            modbus_conf.mbch[i].fcode );

        switch(	modbus_conf.mbch[i].type ) {
            case u16:
            	xprintf_P(PSTR("U16,"));
                break;
            case i16:
                xprintf_P(PSTR("I16,"));
                break;
            case u32:
                xprintf_P(PSTR("U32,"));
                break;
            case i32:
                xprintf_P(PSTR("I32,"));
                break;
            case FLOAT:
                xprintf_P(PSTR("FLT,"));
            	break;
        }

        switch(	modbus_conf.mbch[i].codec ) {
            case CODEC0123:
                xprintf_P(PSTR("c0123,"));
                break;
            case CODEC1032:
                xprintf_P(PSTR("c1032,"));
                break;
            case CODEC3210:
                xprintf_P(PSTR("c3210,"));
                break;
            case CODEC2301:
                xprintf_P(PSTR("c2301,"));
                break;
        }

        xprintf_P( PSTR("%d]\r\n"), modbus_conf.mbch[i].divisor_p10	);
	}

}
//------------------------------------------------------------------------------
bool modbus_config_enable( char *s_enable)
{
    if (!strcmp_P( strupr(s_enable), PSTR("TRUE"))  ) {
        modbus_conf.enabled = true;
        return (true);
    } else if (!strcmp_P( strupr(s_enable), PSTR("FALSE"))  ) {
        modbus_conf.enabled = false;
        return (true);
    }
    return (false);
}
//------------------------------------------------------------------------------
bool modbus_config_localaddr( char *s_localaddr)
{
    modbus_conf.localaddr = atoi(s_localaddr);
    return (true);
}
//------------------------------------------------------------------------------
bool modbus_config_channel( uint8_t ch, 
        char *s_enable, 
        char *s_name, 
        char *s_sla, 
        char *s_regaddr, 
        char *s_nroregs,
        char *s_fcode,
        char *s_mtype,
        char *s_codec,
        char *s_pow10 )
{
	
    // Todos los datos vienen en decimal !!!
	/*
     * xprintf_P(PSTR("DEBUG MBCH ch:[%d],enable:[%s],name:[%s],sla:[%s],regaddr:[%s],nrecs:[%s],fcode:[%s],mtype:[%s],codec:[%s],pow10:[%s]\r\n"),
			channel,
            s_enable,
			s_name,
			s_sla,
			s_regaddr,
            s_nroregs,
			s_fcode,
			s_mtype,
			s_codec,
			s_pow10 );
	 *
     */
    
//	if (( s_name == NULL ) || ( s_addr == NULL) || (s_sla == NULL) || (s_nro_recds == NULL) || ( s_fcode == NULL) || ( s_divisor_p10 == NULL)  ) {
//		return(false);
//	}
bool retS = false;

	if ( s_name == NULL ) {
		return(retS);
	}

	if ( ( ch >=  0) && ( ch < NRO_MODBUS_CHANNELS ) ) {
 
        //xprintf_P(PSTR("MBDEBUG: channel=%d\r\n"), channel );
 
        // Enable ?
        if (!strcmp_P( strupr(s_enable), PSTR("TRUE"))  ) {
            modbus_conf.mbch[ch].enabled = true;
        } else if (!strcmp_P( strupr(s_enable), PSTR("FALSE"))  ) {
            modbus_conf.mbch[ch].enabled = false;
        }
        
		snprintf_P( modbus_conf.mbch[ch].name, MODBUS_PARAMNAME_LENGTH, PSTR("%s"), s_name );
        //xprintf_P(PSTR("MBDEBUG: name=%s\r\n"), modbus_conf->mbch[channel].name );
                
        if ( s_sla != NULL ) {
            modbus_conf.mbch[ch].slave_address = atoi(s_sla);
            //xprintf_P(PSTR("MBDEBUG: sla=%d\r\n"), modbus_conf->mbch[channel].slave_address );
        }
        
        if ( s_regaddr != NULL ) {
            modbus_conf.mbch[ch].reg_address = atoi(s_regaddr);
            //xprintf_P(PSTR("MBDEBUG: reg_addr=%d\r\n"), modbus_conf->mbch[channel].reg_address );
        }

        if ( s_nroregs != NULL ) {
            modbus_conf.mbch[ch].nro_regs = atoi(s_nroregs);
            //xprintf_P(PSTR("MBDEBUG: nro_regs=%d\r\n"), modbus_conf->mbch[channel].nro_regs );
        }

        if ( s_fcode != NULL ) {
            modbus_conf.mbch[ch].fcode = atoi(s_fcode);
            //xprintf_P(PSTR("MBDEBUG: fcode=%d\r\n"), modbus_conf->mbch[channel].fcode );
        }

        // mTipo
        if ( !strcmp_P( strupr(s_mtype), PSTR("U16"))) {
            modbus_conf.mbch[ch].type = u16;

        } else 	if ( !strcmp_P( strupr(s_mtype), PSTR("I16"))) {
            modbus_conf.mbch[ch].type = i16;

        } else 	if ( !strcmp_P( strupr(s_mtype), PSTR("U32"))) {
            modbus_conf.mbch[ch].type = u32;

        } else 	if ( !strcmp_P( strupr(s_mtype), PSTR("I32"))) {
            modbus_conf.mbch[ch].type = i32;

        } else 	if ( !strcmp_P( strupr(s_mtype), PSTR("FLOAT"))) {
            modbus_conf.mbch[ch].type = FLOAT;

        } else {
            //xprintf_P(PSTR("MODBUS CONFIG ERROR: type [%s]\r\n"), s_mtype );
            return(false);
        }

        //xprintf_P(PSTR("MBDEBUG: mtype=%d\r\n"), modbus_conf->mbch[channel].type );
         
        // Codec
        if ( !strcmp_P( strupr(s_codec), PSTR("C0123"))) {
            modbus_conf.mbch[ch].codec = CODEC0123;

        } else 	if ( !strcmp_P( strupr(s_codec), PSTR("C1032"))) {
            modbus_conf.mbch[ch].codec = CODEC1032;

        } else 	if ( !strcmp_P( strupr(s_codec), PSTR("C3210"))) {
            modbus_conf.mbch[ch].codec = CODEC3210;

        } else 	if ( !strcmp_P( strupr(s_codec), PSTR("C2301"))) {
            modbus_conf.mbch[ch].codec = CODEC2301;

        } else {
            xprintf_P(PSTR("MODBUS CONFIG ERROR: codec [%s]\r\n"), s_codec );
            return(false);
        }
        
        //xprintf_P(PSTR("MBDEBUG: codec=%d\r\n"), modbus_conf->mbch[channel].codec );
 
        
        if ( s_pow10 != NULL ) {
            modbus_conf.mbch[ch].divisor_p10 = atoi(s_pow10);
        }

        //xprintf_P(PSTR("MBDEBUG: pow10=%d\r\n"), modbus_conf->mbch[channel].divisor_p10 );
         
        return(true);
    }
                
    return(false);

}
//------------------------------------------------------------------------------
// PROCESO
//------------------------------------------------------------------------------
void modbus_make_ADU( mbus_CONTROL_BLOCK_t *mbus_cb )
{
	/*
	 * Prepara el ADU ( raw frame ) a trasnmitir con los datos de la variable global mbus_cb.
	 * Calcula el CRC de modo que el frame queda pronto a transmitirse por el serial
	 * Solo utilizamos los mensajes tipo 3,6,10
	 */

uint8_t size = 0;
uint16_t crc;

	// Header:
	memset( mbus_cb->tx_buffer, '\0', MBUS_TXMSG_LENGTH );
	mbus_cb->tx_buffer[0] = mbus_cb->channel.slave_address;								// SLA
	mbus_cb->tx_buffer[1] = mbus_cb->channel.fcode;										// FCODE
	mbus_cb->tx_buffer[2] = (uint8_t) (( mbus_cb->channel.reg_address & 0xFF00  ) >> 8);// DST_ADDR_H
	mbus_cb->tx_buffer[3] = (uint8_t) ( mbus_cb->channel.reg_address & 0x00FF );		// DST_ADDR_L

	// Payload:
	if ( mbus_cb->channel.fcode == 0x03 ) {
		// READ HOLDING REGISTER
		// Hago el ADU de un frame tipo 0x03 ( Read Holding Register )
		mbus_cb->tx_buffer[4] = (uint8_t) ((mbus_cb->channel.nro_regs & 0xFF00 ) >> 8);	// NRO_REG_HI
		mbus_cb->tx_buffer[5] = (uint8_t) ( mbus_cb->channel.nro_regs & 0x00FF );		// NRO_REG_LO
		size = 6;

	} else if ( mbus_cb->channel.fcode == 0x06 ) {
		// WRITE SINGLE REGISTER
		// Hago el ADU de un frame tipo 0x06 ( Write Single Register )
		// El valor a escribir esta en mbus_cb.udata.
		// Escribo solo 2 bytes con esta funcion !!! (i16,u16)
		switch( mbus_cb->channel.codec ) {
		case CODEC0123:
			pv_encoder_f6_c0123( mbus_cb );
			break;
		case CODEC1032:
			pv_encoder_f6_c1032( mbus_cb );
			break;
		case CODEC3210:
			pv_encoder_f6_c3210( mbus_cb );
			break;
		case CODEC2301:
			pv_encoder_f6_c2301( mbus_cb );
			break;
		default:
			return;
		}
		size = 6;

	} else if ( mbus_cb->channel.fcode == 16 ) {
		// WRITE MULTIPLE REGISTER
		// Hago el ADU de un frame tipo 0x10 (16) ( Write Multitple Register )
		// El valor a escribir esta en mbus_cb.udata(float)
		// Escribo 4 bytes.
		// El simulador recibe los bytes en orden.
		// Quantity of Registers: 2 bytes, ( 1 registro o 1 variable de largo 4  )
		// Escribo solo 2 bytes con esta funcion !!! (i16,u16)
		switch( mbus_cb->channel.codec ) {
		case CODEC0123:
			pv_encoder_f16_c0123( mbus_cb );
			break;
		case CODEC1032:
			pv_encoder_f16_c1032( mbus_cb );
			break;
		case CODEC3210:
			pv_encoder_f16_c3210( mbus_cb );
			break;
		case CODEC2301:
			pv_encoder_f16_c2301( mbus_cb );
			break;
		default:
			return;
		}
		size = mbus_cb->tx_size;

	} else {
		// Codigo no implementado
		return;
	}

	// CRC
	crc = pv_modbus_CRC16( mbus_cb->tx_buffer, size );
	mbus_cb->tx_buffer[size++] = (uint8_t)( crc & 0x00FF );			// CRC Low
	mbus_cb->tx_buffer[size++] = (uint8_t)( (crc & 0xFF00) >> 8 );	// CRC High
	mbus_cb->tx_size = size;
	return;

}
//------------------------------------------------------------------------------
void modbus_txmit_ADU( mbus_CONTROL_BLOCK_t *mbus_cb )
{
	// Transmite el frame modbus almcenado en mbus_cb.tx_buffer
	//
	// OJO: En MODBUS, los bytes pueden ser 0x00 y no deben ser interpretados como NULL
	// al trasmitir por lo tanto hay que usar el data_size
	// Debemos cumplir extrictos limites de tiempo por lo que primero logueo y luego
	// muestro el debug.

uint8_t i;

	// Transmite un mensaje MODBUS
	// Habilita el RTS para indicar que transmite
	// Apaga la recepcion. ( El IC 485 copia la tx y rx )

	//xprintf_PD( f_debug, PSTR("MODBUS: TX start\r\n"));

	// Log
	if ( f_debug_modbus ) {
		xprintf_P( PSTR("MODBUS: TX (len=%d)"), mbus_cb->tx_size);
		for ( i = 0 ; i < mbus_cb->tx_size ; i++ ) {
			xprintf_P( PSTR("[0x%02X]"), mbus_cb->tx_buffer[i]);
		}
		xprintf_P( PSTR("\r\n"));
	}

	// Transmito
	// borro buffers y espero 3.5T (9600) = 3.5ms ( START )
	vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );

    // Borro el rxBuffer antes de transmitir
    ptrFuncFlush();
    
	// La funcion xnprintf_MBUS maneja el control de flujo.
	i = xnprintf( FD_MODBUS, (const char *)mbus_cb->tx_buffer, mbus_cb->tx_size );

    if ( f_debug_modbus ) {
        xprintf_P( PSTR("MODBUS: TX end\r\n") );
    }
}
//------------------------------------------------------------------------------
void modbus_decode_ADU ( mbus_CONTROL_BLOCK_t *mbus_cb )
{

	/*
	 * Decodifica c/tipo de respuesta en base al function_code y tipo.
	 * Invoca a los decoders particulares.
	 *
	 */

	//xprintf_PD( f_debug, PSTR("MODBUS: DECODE start\r\n"));

	// Proceso los errores escribiendo un NaN
	if ( mbus_cb->io_status == false ) {
		mbus_cb->udata.raw_value[0] = 0xFF;
		mbus_cb->udata.raw_value[1] = 0xFF;
		mbus_cb->udata.raw_value[2] = 0xFF;
		mbus_cb->udata.raw_value[3] = 0xFF;
		goto quit;
	}

	if ( mbus_cb->channel.fcode == 0x03 ) {

		switch( mbus_cb->channel.codec ) {
		case CODEC0123:
			pv_decoder_f3_c0123( mbus_cb, 3 );
			break;
		case CODEC1032:
			pv_decoder_f3_c1032( mbus_cb, 3 );
			break;
		case CODEC3210:
			pv_decoder_f3_c3210( mbus_cb, 3 );
			break;
		case CODEC2301:
			pv_decoder_f3_c2301( mbus_cb, 3 );
			break;
		default:
			return;
		}


	} else if ( mbus_cb->channel.fcode == 0x06 ) {
		// No se analiza la respuesta ya que es echo
		return;

	} else if ( mbus_cb->channel.fcode == 0x10 ) {
		// No se analiza la respuesta ya que es echo
		return;
	} else {
		// Caso no implementado
		return;
	}

quit:

    if ( f_debug_modbus ) {
        xprintf_P( PSTR("MODBUS: DECODE (MSB)b3[0x%02X] b2[0x%02X] b1[0x%02X] b0[0x%02X](LSB)\r\n"),
			mbus_cb->udata.raw_value[3], mbus_cb->udata.raw_value[2], mbus_cb->udata.raw_value[1], mbus_cb->udata.raw_value[0]);
    }

	return;

}
//------------------------------------------------------------------------------
void modbus_rcvd_ADU( mbus_CONTROL_BLOCK_t *mbus_cb )
{
	// Recibe del auxBuffer un frame modbus y lo almacena en mbus_data_s.rx_buffer
	//
	// https://stackoverflow.com/questions/13254432/modbus-is-there-a-maximum-time-a-device-can-take-to-respond
	// Esperamos la respuesta 1s y si no llego damos timeout

	// Lee el buffer de recepcion del puerto AUX (MODBUS )
	// Es por poleo de modo que una vez que transmiti un frame, debo esperar un tiempo que llegue la respuesta
	// y luego usar esta funcion para decodificar la lectura.

	// Para copiar buffers no puedo usar strcpy porque pueden haber datos en 0.
	// Copio byte a byte con limite

	// El punto clave esta en la recepcion por medio de las funciones de aux !!!

	/*
	 *  Para ser efectivo en cuanto espero, el procedimiento es el siguiente:
	 *  Espero 100 ms iniciales
	 *  Leo el aux_buffer.ptr y lo guardo.
	 *  Cada 100 ms leo de nuevo el ptr. Si cambio es que estan viniendo datos.
	 *  Si no cambio, salgo.
	 *
	 */

uint16_t crc_rcvd;
uint16_t crc_calc;
uint8_t i;
int8_t lBytes, rBytes;
int8_t timeout_counts;
char *p;
int max_rx_bytes;


	//xprintf_PD( f_debug, PSTR("MODBUS: RX start\r\n"));

	rBytes = -1;
	lBytes = -1;
	timeout_counts = 50;	// 20 x 50ms: espero 1 sec

	// Espero de a 50ms y voy contando lo caracteres que llegan
	// hasta que no lleguen mas.
    // Se van almacenando en el commsA_buffer.
	while ( timeout_counts-- > 0 ) {

        vTaskDelay( ( TickType_t)( 50 / portTICK_PERIOD_MS ) );

		// Vemos si vinieron mas caracteres.
		rBytes = ptrFuncGetCount();
		if ( rBytes != lBytes ) {
			lBytes = rBytes;
		} else {
			// No se movio. pero ya recibi algo: Salgo
			if (lBytes > 0) {
				break;
			}
		}
	}

	// Leo el aux_rx_buffer y lo dejo en el modbus_rx_buffer
	memset( mbus_cb->rx_buffer, '\0', MBUS_RXMSG_LENGTH );
    // Copio el buffer con los datos recibidos al rx_buffer. Como pueden
    // haber datos en 0x00 no uso strcpy.
    p = ptrFuncRXBufferInit();

    
    // Control para no hacer overflow de buffer !!!
    max_rx_bytes = ptrFuncGetCount();
    if ( f_debug_modbus ) {
        xprintf_P( PSTR("DEBUG: MODBUS: RX_RCVD=%d\r\n"), max_rx_bytes );
    }
    if ( max_rx_bytes > MBUS_RXMSG_LENGTH ) {
        max_rx_bytes = MBUS_RXMSG_LENGTH;
        xprintf_P( PSTR("DEBUG: MODBUS: RX_ERR\r\n") );
    }
    
    //for (i=0; i < ptrFuncGetCount(); i++ ) {
    for (i=0; i < max_rx_bytes; i++ ) {
		mbus_cb->rx_buffer[i] = *p;
        //xprintf_P( PSTR("[0x%02X]"), mbus_cb->rx_buffer[i]);
        p++;
	}
    //mbus_cb->rx_size = ptrFuncGetCount();
    mbus_cb->rx_size = i;

    // Ya lo copie: borro el buffer !!!!
    ptrFuncFlush();
    
    if ( f_debug_modbus ) {
        xprintf_P( PSTR("MODBUS: RX rx_size=%d\r\n"), mbus_cb->rx_size );
    }
    
	// Paso 1: Log
	if ( f_debug_modbus ) {
		xprintf_P( PSTR("MODBUS: RX (len=%d):"), mbus_cb->rx_size);
		for ( i = 0 ; i < mbus_cb->rx_size; i++ ) {
			xprintf_P( PSTR("[0x%02X]"), mbus_cb->rx_buffer[i]);
		}
		xprintf_P( PSTR("\r\n"));
	}

	// Paso 2: Controlo el largo.
	if ( mbus_cb->rx_size < 3) {
		// Timeout error:
		xprintf_P( PSTR("MODBUS: RX TIMEOUT ERROR\r\n"));
		mbus_cb->io_status = false;
		goto quit;
	}

	// Pass3: Calculo y verifico el CRC
	crc_calc = pv_modbus_CRC16( mbus_cb->rx_buffer, (mbus_cb->rx_size - 2) );
	crc_rcvd = mbus_cb->rx_buffer[mbus_cb->rx_size - 2] + ( mbus_cb->rx_buffer[mbus_cb->rx_size - 1] << 8 );

	if ( crc_calc != crc_rcvd) {
		xprintf_P( PSTR("MODBUS: RX CRC ERROR: rx[0x%02x], calc[0x%02x]\r\n\0"), crc_rcvd, crc_calc);
		// De acuerdo al protocolo se ignora el frame recibido con errores CRC
		mbus_cb->io_status = false;
		goto quit;
	}

	mbus_cb->io_status = true;

quit:

	return;

}
//------------------------------------------------------------------------------
void modbus_read( float modbus_values[] )
{
	// Lee todos los canales modbus configurados y deja los valores en el correspondiente
	// luegar del array del parametro.
	// Los nombres de los canales deben ser #X y estar en order. Al primer canal X, sale. !!!

uint8_t ch;

    // EL modbus debe estar habilitado
    if ( ! modbus_conf.enabled ) {
        return;
    }

	// Poleo
	for ( ch = 0; ch < NRO_MODBUS_CHANNELS; ch++) {
		// Si un canal no esta definido, salgo
        if (modbus_conf.mbch[ch].enabled ) {
            // Leemos el canal
        	modbus_values[ch] = modbus_read_channel ( ch );
        }
    }

	return;
    
}
//------------------------------------------------------------------------------
float modbus_read_channel ( uint8_t ch )
{
	/*
	 *  Poleo un canal.
	 *  La funcion modbus_io me devuelve el valor leido en un hreg.
	 *  Dependiendo del tipo es como lo interpreto.
	 *  SIEMPRE lo convierto a float para devolverlo !!!.
	 */

float pvalue = 0.0;

    if ( f_debug_modbus ) {
        xprintf_P( PSTR("MODBUS: CHPOLL %02d START\r\n"),ch );
    }

	// Preparo el registro CONTROL BLOCK con los datos del canal
	// Son operaciones de READ que siempre devuelven floats.
    memset( &mbus_cb, '\0', sizeof(mbus_CONTROL_BLOCK_t));
	memcpy( &mbus_cb.channel, &modbus_conf.mbch[ch], sizeof(modbus_channel_conf_t));
	//
	modbus_io( &mbus_cb );
	//
	// Siempre retorno un float

	// Si hubo un error, pongo un NaN y salgo
	if (mbus_cb.io_status == false) {
		pvalue = mbus_cb.udata.float_value;
		goto quit;
	}

	// La operacion de io es exitosa.
	if ( mbus_cb.channel.type == FLOAT) {
		pvalue = mbus_cb.udata.float_value;

	} else if (mbus_cb.channel.type == i16 ) {
		// En este caso importa el parametro divisor_p10 ya que convierte al entero en float
		pvalue = 1.0 * mbus_cb.udata.i16_value / pow(10, mbus_cb.channel.divisor_p10 );

	} else if (mbus_cb.channel.type == u16 ) {
		pvalue = 1.0 * mbus_cb.udata.u16_value / pow(10, mbus_cb.channel.divisor_p10 );

	} else if (mbus_cb.channel.type == u32 ) {
		pvalue = 1.0 * mbus_cb.udata.u32_value / pow(10, mbus_cb.channel.divisor_p10 );

	} else if (mbus_cb.channel.type == i32 ) {
		pvalue = 1.0 * mbus_cb.udata.i32_value / pow(10, mbus_cb.channel.divisor_p10 );

	}

quit:

	//modbus_print( DF_MBUS, &mbus_cb );
    if ( f_debug_modbus ) {
        xprintf_P( PSTR("MODBUS: pvalue=%0.3f\r\n"), pvalue);
        xprintf_P( PSTR("MODBUS: CHPOLL END\r\n"));
    }
	return( pvalue );

}
//------------------------------------------------------------------------------
void modbus_io( mbus_CONTROL_BLOCK_t *mbus_cb )
{
	/*
	 * En mbus_cb.io_status tenemos el resultado de la operacion
	 */

	mbus_cb->io_status = false;
	//
	modbus_make_ADU ( mbus_cb);
	modbus_txmit_ADU( mbus_cb );

	// Espero la respuesta
	//vTaskDelay( (portTickType)( systemVars.modbus_conf.waiting_poll_time / portTICK_RATE_MS ) );

	modbus_rcvd_ADU( mbus_cb );
	modbus_decode_ADU ( mbus_cb );

	return;
}
//------------------------------------------------------------------------------
// FUNCIONES AUXILIARES
//------------------------------------------------------------------------------
uint16_t pv_modbus_CRC16( uint8_t *msg, uint8_t msg_size )
{

uint16_t crc = 0xFFFF;
uint16_t pos;
uint16_t data;
int i;

	for ( pos = 0; pos < msg_size; pos++) {
		data = (uint16_t)*msg++;
		crc ^= data;          // XOR byte into least sig. byte of crc

		for (i = 8; i != 0; i--) {    // Loop over each bit
			if ((crc & 0x0001) != 0) {      // If the LSB is set
				crc >>= 1;                    // Shift right and XOR 0xA001
				crc ^= 0xA001;
			} else {                            // Else LSB is not set
				crc >>= 1;                    // Just shift right
			}
		}
	}
	// Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
	return crc;
}
//------------------------------------------------------------------------------
// DECODERS
/*
 *
 * https://betterexplained.com/articles/understanding-big-and-little-endian-byte-order/
 *
 * En la decodificacion de los enteros no hay problema porque los flowmeters y los PLC
 * mandan el MSB primero.
 * En el caso de los float, solo los mandan los PLC y ahi hay un swap de los bytes por
 * lo que para decodificarlo necesito saber que los 4 bytes corresponden a un float!!
 *
 * MODBUS es BIG_ENDIAN: Cuando se manda un nro de varios bytes, el MSB se manda primero
 * MSB ..... LSB
 * Big Endian: El MSB se pone en el byte de la direccion mas baja:
 * Little Endian: El LSB se pone en el byte de la direccion mas baja.
 *
 * El decoder indica el orden de los bytes recibidos.
 * Al recibirlos van a una memoria del datalogger que codifica el LSB en la posicion mas baja (b0)
 * El codec debe indicar en que orden se reciben los bytes.
 * Ej: c3210 indica que primero se recibe el byte3, byte2, byte1 y byte0
 *
 */
//------------------------------------------------------------------------------
void pv_decoder_f3_c3210( mbus_CONTROL_BLOCK_t *mbus_cb, uint8_t payload_ptr )
{
	/*
	 * c3210 significa decodificar los bytes en el orden natural.
	 * En una variable de memoria del datalogger, un valor de varios bytes se almacena
	 * con el LSB en la posicion 0.
	 * Cuando llegan los bytes por Modbus, el ultimo byte del payload es el LSB
	 * En el string del payload, la posicion ptr0 -> byte3, ptr1->byte2, ptr2->byte1, ptr0->byte0
	 */
//uint8_t payload_ptr = 3;	// Posicion en el rxbuffer donde empieza el payload

    if ( f_debug_modbus ) {
        xprintf_P(PSTR("MODBUS: codec_f3_c3210\r\n"));
    }
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		// El payload tiene 2 bytes.
		mbus_cb->udata.raw_value[3] = 0x00;
		mbus_cb->udata.raw_value[2] = 0x00;
		mbus_cb->udata.raw_value[1] = mbus_cb->rx_buffer[payload_ptr + 0];
		mbus_cb->udata.raw_value[0] = mbus_cb->rx_buffer[payload_ptr + 1];
	} else {
		// El payload tiene 4 bytes.
		mbus_cb->udata.raw_value[3] = mbus_cb->rx_buffer[payload_ptr + 0];
		mbus_cb->udata.raw_value[2] = mbus_cb->rx_buffer[payload_ptr + 1];
		mbus_cb->udata.raw_value[1] = mbus_cb->rx_buffer[payload_ptr + 2];
		mbus_cb->udata.raw_value[0] = mbus_cb->rx_buffer[payload_ptr + 3];
	}
	return;
}
//------------------------------------------------------------------------------
void pv_decoder_f3_c0123( mbus_CONTROL_BLOCK_t *mbus_cb, uint8_t payload_ptr )
{
	/*
	 * c4321 significa decodificar los bytes en orden inverso.
	 * La referencia es la memoria del datalogger que para interpretar un valor, requiere
	 * que el LSB se almacene en la posicion mas baja (b0)
	 * En esta codificacion, lo que hago es poner los datos que me llegan del modbus
	 * en un orden inverso al orden natural ( c0123 )
	 * En el string del payload, la posicion ptr0 -> byte0, ptr1->byte1, ptr2->byte2, ptr0->byte3
	 *
	 */

//uint8_t payload_ptr = 3;	// Posicion en el rxbuffer donde empieza el payload

    if ( f_debug_modbus ) {
        xprintf_P(PSTR("MODBUS: codec_f3_c0123\r\n"));
    }
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->udata.raw_value[3] = 0x00;
		mbus_cb->udata.raw_value[2] = 0x00;
		mbus_cb->udata.raw_value[1] = mbus_cb->rx_buffer[payload_ptr + 1];
		mbus_cb->udata.raw_value[0] = mbus_cb->rx_buffer[payload_ptr + 0];

	} else {
		mbus_cb->udata.raw_value[3] = mbus_cb->rx_buffer[payload_ptr + 3];
		mbus_cb->udata.raw_value[2] = mbus_cb->rx_buffer[payload_ptr + 2];
		mbus_cb->udata.raw_value[1] = mbus_cb->rx_buffer[payload_ptr + 1];
		mbus_cb->udata.raw_value[0] = mbus_cb->rx_buffer[payload_ptr + 0];
	}
	return;

}
//------------------------------------------------------------------------------
void pv_decoder_f3_c1032( mbus_CONTROL_BLOCK_t *mbus_cb, uint8_t payload_ptr )
{

//uint8_t payload_ptr = 3;	// Posicion en el rxbuffer donde empieza el payload

    if ( f_debug_modbus ) {
        xprintf_P(PSTR("MODBUS: codec_f3_c1032\r\n"));
    }
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->udata.raw_value[3] = 0x00;
		mbus_cb->udata.raw_value[2] = 0x00;
		mbus_cb->udata.raw_value[1] = mbus_cb->rx_buffer[payload_ptr + 0];
		mbus_cb->udata.raw_value[0] = mbus_cb->rx_buffer[payload_ptr + 1];
	} else {
		mbus_cb->udata.raw_value[3] = mbus_cb->rx_buffer[payload_ptr + 2];
		mbus_cb->udata.raw_value[2] = mbus_cb->rx_buffer[payload_ptr + 3];
		mbus_cb->udata.raw_value[1] = mbus_cb->rx_buffer[payload_ptr + 0];
		mbus_cb->udata.raw_value[0] = mbus_cb->rx_buffer[payload_ptr + 1];
	}
	return;
}
//------------------------------------------------------------------------------
void pv_decoder_f3_c2301( mbus_CONTROL_BLOCK_t *mbus_cb, uint8_t payload_ptr )
{

//uint8_t payload_ptr = 3;	// Posicion en el rxbuffer donde empieza el payload

    if ( f_debug_modbus ) {
        xprintf_P(PSTR("MODBUS: codec_f3_c2301\r\n"));
    }
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->udata.raw_value[3] = 0x00;
		mbus_cb->udata.raw_value[2] = 0x00;
		mbus_cb->udata.raw_value[1] = mbus_cb->rx_buffer[payload_ptr + 1];
		mbus_cb->udata.raw_value[0] = mbus_cb->rx_buffer[payload_ptr + 0];

	} else {
		mbus_cb->udata.raw_value[3] = mbus_cb->rx_buffer[payload_ptr + 1];
		mbus_cb->udata.raw_value[2] = mbus_cb->rx_buffer[payload_ptr + 0];
		mbus_cb->udata.raw_value[1] = mbus_cb->rx_buffer[payload_ptr + 3];
		mbus_cb->udata.raw_value[0] = mbus_cb->rx_buffer[payload_ptr + 2];
	}
	return;
}
//------------------------------------------------------------------------------
/*
 * ENCODERS
 * Dado una variable int,long,float en la memoria del datalogger (b0=LSB), debo
 * ver como reordeno los bytes para transmitirlos al modbus.
 * En la funcion 0x06 solo se escriben 2 bytes.
 */
//------------------------------------------------------------------------------
void pv_encoder_f6_c0123(mbus_CONTROL_BLOCK_t *mbus_cb )
{

	mbus_cb->tx_buffer[4] = mbus_cb->udata.raw_value[0];	// DATA_VAL_HI
	mbus_cb->tx_buffer[5] = mbus_cb->udata.raw_value[1];	// DATA_VAL_LO
	return;
}
//------------------------------------------------------------------------------
void pv_encoder_f6_c2301(mbus_CONTROL_BLOCK_t *mbus_cb )
{

	mbus_cb->tx_buffer[4] = mbus_cb->udata.raw_value[0];	// DATA_VAL_HI
	mbus_cb->tx_buffer[5] = mbus_cb->udata.raw_value[1];	// DATA_VAL_LO
	return;
}
//------------------------------------------------------------------------------
void pv_encoder_f6_c3210(mbus_CONTROL_BLOCK_t *mbus_cb )
{

	mbus_cb->tx_buffer[4] = mbus_cb->udata.raw_value[1];	// DATA_VAL_HI
	mbus_cb->tx_buffer[5] = mbus_cb->udata.raw_value[0];	// DATA_VAL_LO
	return;
}
//------------------------------------------------------------------------------
void pv_encoder_f6_c1032(mbus_CONTROL_BLOCK_t *mbus_cb )
{

	mbus_cb->tx_buffer[4] = mbus_cb->udata.raw_value[1];	// DATA_VAL_HI
	mbus_cb->tx_buffer[5] = mbus_cb->udata.raw_value[0];	// DATA_VAL_LO
	return;
}
//------------------------------------------------------------------------------
void pv_encoder_f16_c0123(mbus_CONTROL_BLOCK_t *mbus_cb )
{

	// Qty.of registers ( 2 bytes ). 1 registro = 2 bytes
	mbus_cb->tx_buffer[4] = 0;
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->tx_buffer[5] = 1;
	} else {
		mbus_cb->tx_buffer[5] = 2;
	}
	//
	// Byte count ,1 byte ( 1 float = 4 bytes , 2 x N => N=2)
	mbus_cb->tx_buffer[6] = 2 * mbus_cb->tx_buffer[5];

	// LA POSICION ES FIJA EN WRITE: LA ACEPTADA POR LOS PLC KINCO
	// El PCL recibe los bytes en diferente orden !!!.
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->tx_buffer[ 7 ] = mbus_cb->udata.raw_value[0];	// DATA
		mbus_cb->tx_buffer[ 8 ] = mbus_cb->udata.raw_value[1];
		mbus_cb->tx_size = 9;
	} else {
		// Son 4 bytes.
		mbus_cb->tx_buffer[ 7 ] = mbus_cb->udata.raw_value[0];	// DATA
		mbus_cb->tx_buffer[ 8 ] = mbus_cb->udata.raw_value[1];
		mbus_cb->tx_buffer[ 9 ] = mbus_cb->udata.raw_value[2];
		mbus_cb->tx_buffer[ 10 ] = mbus_cb->udata.raw_value[3];
		mbus_cb->tx_size = 11;
	}
	return;
}
//------------------------------------------------------------------------------
void pv_encoder_f16_c2301(mbus_CONTROL_BLOCK_t *mbus_cb )
{


	// Qty.of registers ( 2 bytes ). 1 registro = 2 bytes
	mbus_cb->tx_buffer[4] = 0;
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->tx_buffer[5] = 1;
	} else {
		mbus_cb->tx_buffer[5] = 2;
	}
	//
	// Byte count ,1 byte ( 1 float = 4 bytes , 2 x N => N=2)
	mbus_cb->tx_buffer[6] = 2 * mbus_cb->tx_buffer[5];

	// LA POSICION ES FIJA EN WRITE: LA ACEPTADA POR LOS PLC KINCO
	// El PCL recibe los bytes en diferente orden !!!.
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->tx_buffer[ 7 ] = mbus_cb->udata.raw_value[0];	// DATA
		mbus_cb->tx_buffer[ 8 ] = mbus_cb->udata.raw_value[1];
		mbus_cb->tx_size = 9;
	} else {
		mbus_cb->tx_buffer[ 7]  = mbus_cb->udata.raw_value[2];	// DATA
		mbus_cb->tx_buffer[ 8 ] = mbus_cb->udata.raw_value[3];
		mbus_cb->tx_buffer[ 9 ] = mbus_cb->udata.raw_value[0];
		mbus_cb->tx_buffer[ 10 ] = mbus_cb->udata.raw_value[1];
		mbus_cb->tx_size = 11;
	}

	return;
}
//------------------------------------------------------------------------------
void pv_encoder_f16_c3210(mbus_CONTROL_BLOCK_t *mbus_cb )
{

	// Qty.of registers ( 2 bytes ). 1 registro = 2 bytes
	mbus_cb->tx_buffer[4] = 0;
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->tx_buffer[5] = 1;
	} else {
		mbus_cb->tx_buffer[5] = 2;
	}
	//
	// Byte count ,1 byte ( 1 float = 4 bytes , 2 x N => N=2)
	mbus_cb->tx_buffer[6] = 2 * mbus_cb->tx_buffer[5];

	// LA POSICION ES FIJA EN WRITE: LA ACEPTADA POR LOS PLC KINCO
	// El PCL recibe los bytes en diferente orden !!!.
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->tx_buffer[ 7 ] = mbus_cb->udata.raw_value[1];	// DATA
		mbus_cb->tx_buffer[ 8 ] = mbus_cb->udata.raw_value[0];
		mbus_cb->tx_size = 9;
	} else {
		// Son 4 bytes.
		mbus_cb->tx_buffer[ 7 ] = mbus_cb->udata.raw_value[3];	// DATA
		mbus_cb->tx_buffer[ 8 ] = mbus_cb->udata.raw_value[2];
		mbus_cb->tx_buffer[ 9 ] = mbus_cb->udata.raw_value[1];
		mbus_cb->tx_buffer[ 10 ] = mbus_cb->udata.raw_value[0];
		mbus_cb->tx_size = 11;
	}

	return;
}
//------------------------------------------------------------------------------
void pv_encoder_f16_c1032(mbus_CONTROL_BLOCK_t *mbus_cb )
{

	// Qty.of registers ( 2 bytes ). 1 registro = 2 bytes
	mbus_cb->tx_buffer[4] = 0;
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->tx_buffer[5] = 1;
	} else {
		mbus_cb->tx_buffer[5] = 2;
	}
	//
	// Byte count ,1 byte ( 1 float = 4 bytes , 2 x N => N=2)
	mbus_cb->tx_buffer[6] = 2 * mbus_cb->tx_buffer[5];

	// LA POSICION ES FIJA EN WRITE: LA ACEPTADA POR LOS PLC KINCO
	// El PCL recibe los bytes en diferente orden !!!.
	if ( ( mbus_cb->channel.type == u16 ) || ( mbus_cb->channel.type == i16) ) {
		mbus_cb->tx_buffer[ 7 ] = mbus_cb->udata.raw_value[1];	// DATA
		mbus_cb->tx_buffer[ 8 ] = mbus_cb->udata.raw_value[0];
		mbus_cb->tx_size = 9;
	} else {
		// Son 4 bytes.
		mbus_cb->tx_buffer[ 7 ] = mbus_cb->udata.raw_value[1];	// DATA
		mbus_cb->tx_buffer[ 8 ] = mbus_cb->udata.raw_value[0];
		mbus_cb->tx_buffer[ 9 ] = mbus_cb->udata.raw_value[3];
		mbus_cb->tx_buffer[ 10 ] = mbus_cb->udata.raw_value[2];
		mbus_cb->tx_size = 11;
	}

	return;
}
//------------------------------------------------------------------------------
// FUNCIONES DE TESTING
//------------------------------------------------------------------------------
void modbus_test_genpoll(char *arg_ptr[16] )
{

	// Recibe el argv con los datos en char hex para trasmitir el frame.
	// El formato es: slaaddr,regaddr,nro_regs,fcode,type,codec
	// write modbus genpoll F 9 3 4118 2 c3212
	// TX: (len=8):[0x09][0x03][0x10][0x16][0x00][0x02][0x20][0x47]


	xprintf_P( PSTR("MODBUS: GENPOLL START\r\n"));

	if ( arg_ptr[8] == NULL ) {
		xprintf_P(PSTR("Error de argumentos\r\n"));
		return;
	}

	mbus_cb.channel.slave_address = atoi(arg_ptr[3]);
	mbus_cb.channel.reg_address = atoi(arg_ptr[4]);
	mbus_cb.channel.nro_regs = atoi(arg_ptr[5]);
	mbus_cb.channel.fcode = atoi(arg_ptr[6]);

	// Preparo el registro con los datos del canal
	// TYPE
	if ( strcmp ( strupr(arg_ptr[7]), "U16" ) == 0 ) {
		mbus_cb.channel.type = u16;
	} else 	if ( strcmp ( strupr(arg_ptr[7]), "I16" ) == 0 ) {
		mbus_cb.channel.type = i16;
	} else 	if ( strcmp ( strupr(arg_ptr[7]), "I32" ) == 0 ) {
		mbus_cb.channel.type = i32;
	} else 	if ( strcmp ( strupr(arg_ptr[7]), "U32" ) == 0 ) {
		mbus_cb.channel.type = u32;
	} else 	if ( strcmp ( strupr(arg_ptr[7]), "FLOAT" ) == 0 ) {
		mbus_cb.channel.type = FLOAT;
	} else {
		xprintf_P(PSTR("ERROR: tipo no soportado\r\n"));
		return;
	}

	// CODEC
	if ( strcmp ( strupr(arg_ptr[8]), "C0123" ) == 0 ) {
		mbus_cb.channel.codec = CODEC0123;
	} else 	if ( strcmp ( strupr(arg_ptr[8]), "C1032" ) == 0 ) {
		mbus_cb.channel.codec = CODEC1032;
	} else 	if ( strcmp ( strupr(arg_ptr[8]), "C3210" ) == 0 ) {
		mbus_cb.channel.codec = CODEC3210;
	} else 	if ( strcmp ( strupr(arg_ptr[8]), "C2301" ) == 0 ) {
		mbus_cb.channel.codec = CODEC2301;
	} else {
		xprintf_P(PSTR("ERROR: codec no soportado\r\n"));
		return;
	}

	mbus_cb.channel.divisor_p10 = 0;
	//
	modbus_io( &mbus_cb );
	//
	modbus_print_value( &mbus_cb );
	xprintf_P(PSTR("MODBUS: GENPOLL END\r\n"));

}
//------------------------------------------------------------------------------
void modbus_test_channel(char *s_channel )
{
	// Hace un poleo de un canal modbus definido en el datalogger

uint8_t ch;

	ch = atoi(s_channel);

	if ( ch >= NRO_MODBUS_CHANNELS ) {
		xprintf_P(PSTR("ERROR: Nro.canal < %d\r\n"), NRO_MODBUS_CHANNELS);
		return;
	}

	if ( ! modbus_conf.mbch[ch].enabled ) {
		xprintf_P(PSTR("ERROR: Canal no definido (X)\r\n"));
		return;
	}

	modbus_read_channel(ch );

}
//------------------------------------------------------------------------------
void modbus_print_value( mbus_CONTROL_BLOCK_t *mbus_cb )
{

	// Imprime en la terminal el valor leido en una operacion de modbus.
	// Se usa en las funciones de mbustest.

float pvalue;

	if ( mbus_cb->io_status == false ) {
		xprintf_P( PSTR("MODBUS: VALUE NaN\r\n"));
		return;
	}

	// La operacion de io es exitosa.
	if ( mbus_cb->channel.type == FLOAT) {
		xprintf_P( PSTR("MODBUS: VALUE %.03f (F)\r\n"), mbus_cb->udata.float_value);

	} else if (mbus_cb->channel.type == i16 ) {
		// En este caso importa el parametro divisor_p10 ya que convierte al entero en float
		pvalue = 1.0 * mbus_cb->udata.i16_value / pow(10, mbus_cb->channel.divisor_p10 );
		xprintf_P( PSTR("MODBUS: VALUE %d (i16), %.03f\r\n"), mbus_cb->udata.i16_value, pvalue );

	} else if (mbus_cb->channel.type == u16 ) {
		pvalue = 1.0 * mbus_cb->udata.u16_value / pow(10, mbus_cb->channel.divisor_p10 );
		xprintf_P( PSTR("MODBUS: VALUE %u (u16), %.03f\r\n"), mbus_cb->udata.u16_value, pvalue );

	} else if (mbus_cb->channel.type == u32 ) {
		pvalue = 1.0 * ( mbus_cb->udata.u32_value ) / pow(10, mbus_cb->channel.divisor_p10 );
		xprintf_P( PSTR("MODBUS: VALUE %lu (u32), %.03f\r\n"), mbus_cb->udata.u32_value, pvalue );

	} else if (mbus_cb->channel.type == i32 ) {
		pvalue = 1.0 * ( mbus_cb->udata.i32_value ) / pow(10, mbus_cb->channel.divisor_p10 );
		xprintf_P( PSTR("MODBUS: VALUE %ld (i32), %.03f\r\n"), mbus_cb->udata.i32_value, pvalue );
	}
}
//------------------------------------------------------------------------------
uint8_t modbus_hash( uint8_t f_hash(uint8_t seed, char ch )  )
{
     /*
      * Calculo el hash de la configuracion de modbus.
      */
    
uint8_t i,j;
uint8_t hash = 0;
char *p;
uint8_t hash_buffer[64];

   // Calculo el hash de la configuracion modbus

    memset(hash_buffer, '\0', sizeof(hash_buffer) );
    j = 0;
    if (modbus_conf.enabled ) {
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("[TRUE,%02d]"),modbus_conf.localaddr);
    } else {
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("[FALSE,%02d]"),modbus_conf.localaddr);
    }
    p = (char *)hash_buffer;
    while (*p != '\0') {
        hash = f_hash(hash, *p++);
    }
    //xprintf_P(PSTR("HASH_MODBUS:%s, hash=%d\r\n"), hash_buffer, hash );   
    
    for(i=0; i < NRO_MODBUS_CHANNELS; i++) {
        memset(hash_buffer, '\0', sizeof(hash_buffer) );
        j = 0;
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("[M%d:"), i);
        
        if (modbus_conf.mbch[i].enabled ) {
            j += sprintf_P( (char *)&hash_buffer[j], PSTR("TRUE,"));
        } else {
            j += sprintf_P( (char *)&hash_buffer[j], PSTR("FALSE,"));
        }
        
        j += sprintf_P( (char *)&hash_buffer[j], PSTR("%s,%02d,%04d,%02d,%02d,"),
				modbus_conf.mbch[i].name,
				modbus_conf.mbch[i].slave_address,
				modbus_conf.mbch[i].reg_address,
				modbus_conf.mbch[i].nro_regs,
				modbus_conf.mbch[i].fcode
			); 
        
        // TYPE
		switch( modbus_conf.mbch[i].type ) {
		case u16:
			j += sprintf_P( (char *)&hash_buffer[j], PSTR("U16,"));
			break;
		case i16:
			j += sprintf_P( (char *)&hash_buffer[j], PSTR("I16,"));
			break;
		case u32:
			j += sprintf_P( (char *)&hash_buffer[j], PSTR("U32,"));
			break;
		case i32:
			j += sprintf_P( (char *)&hash_buffer[j], PSTR("I32,"));
			break;
		case FLOAT:
			j += sprintf_P( (char *)&hash_buffer[j], PSTR("FLOAT,"));
			break;
		}

		// CODEC
		switch( modbus_conf.mbch[i].codec ) {
		case CODEC0123:
			j += sprintf_P( (char *)&hash_buffer[j], PSTR("C0123,"));
			break;
		case CODEC1032:
			j += sprintf_P( (char *)&hash_buffer[j], PSTR("C1032,"));
			break;
		case CODEC3210:
			j += sprintf_P( (char *)&hash_buffer[j], PSTR("C3210,"));
			break;
		case CODEC2301:
			j += sprintf_P( (char *)&hash_buffer[j], PSTR("C2301,"));
			break;
		}

		j += sprintf_P( (char *)&hash_buffer[j], PSTR("%02d]"), modbus_conf.mbch[i].divisor_p10 );

        p = (char *)hash_buffer;
        while (*p != '\0') {
            hash = f_hash(hash, *p++);
        }
        //xprintf_P(PSTR("HASH_MODBUS:%s, hash=%d\r\n"), hash_buffer, hash );
    }
    return(hash);
}

//------------------------------------------------------------------------------
