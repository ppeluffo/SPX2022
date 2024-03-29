#include "ainputs.h"

// Factor por el que hay que multitplicar el valor raw de los INA para tener
// una corriente con una resistencia de 7.32 ohms.
// Surge que 1LSB corresponde a 40uV y que la resistencia que ponemos es de 7.32 ohms
// 1000 / 7.32 / 40 = 183 ;
#define INA_FACTOR  183

static bool sensores_prendidos = false;

static bool f_debug_ainputs;

// Los caudales los almaceno en un RB y lo que doy es el promedio !!
typedef struct {
    float ain;
} t_ain_s;

#define MAX_RB_AIN_STORAGE_SIZE  10
t_ain_s ain_storage_0[MAX_RB_AIN_STORAGE_SIZE];
t_ain_s ain_storage_1[MAX_RB_AIN_STORAGE_SIZE];
t_ain_s ain_storage_2[MAX_RB_AIN_STORAGE_SIZE];
rBstruct_s ain_RB_0,ain_RB_1,ain_RB_2;

static uint8_t max_rb_ain_storage_size;

//------------------------------------------------------------------------------
void ainputs_init(uint8_t samples_count)
{
    /*
     * Inicializa el INA.
     * Como usa el FRTOS, debe correrse luego que este este corriendo.!!!
     */
    
    INA_config(CONF_INA_AVG128);
    INA_config(CONF_INA_SLEEP);
    
    max_rb_ain_storage_size = samples_count;
    
    if ( max_rb_ain_storage_size > MAX_RB_AIN_STORAGE_SIZE ) {
        max_rb_ain_storage_size = MAX_RB_AIN_STORAGE_SIZE;
    }
    
    rBstruct_CreateStatic ( 
        &ain_RB_0, 
        &ain_storage_0, 
        max_rb_ain_storage_size, 
        sizeof(t_ain_s), 
        true  
    );
    
    rBstruct_CreateStatic ( 
        &ain_RB_1, 
        &ain_storage_1, 
        max_rb_ain_storage_size, 
        sizeof(t_ain_s), 
        true  
    );
        
    rBstruct_CreateStatic ( 
        &ain_RB_2, 
        &ain_storage_2, 
        max_rb_ain_storage_size, 
        sizeof(t_ain_s), 
        true  
    );
}
//------------------------------------------------------------------------------
void ainputs_awake(void)
{
    /*
     * Saca al INA del estado de reposo LOW-POWER
     */
    INA_awake();
}
//------------------------------------------------------------------------------
void ainputs_sleep(void)
{
    /*
     * Pone al INA en estado LOW-POWER
     */
    INA_sleep();
}
//------------------------------------------------------------------------------
bool ainputs_config_channel( uint8_t channel, ainputs_conf_t *ainputs_conf, char *s_aname,char *s_imin,char *s_imax,char *s_mmin,char *s_mmax,char *s_offset )
{

	/*
     * Configura los canales analogicos. Es usada tanto desde el modo comando como desde el modo online por gprs.
     */ 


bool retS = false;

	if ( s_aname == NULL ) {
		return(retS);
	}

	if ( ( channel >=  0) && ( channel < NRO_ANALOG_CHANNELS ) ) {

		snprintf_P( ainputs_conf[channel].name, AIN_PARAMNAME_LENGTH, PSTR("%s\0"), s_aname );

		if ( s_imin != NULL ) {
			if ( ainputs_conf[channel].imin != atoi(s_imin) ) {
				ainputs_conf[channel].imin = atoi(s_imin);
			}
		}

		if ( s_imax != NULL ) {
			if ( ainputs_conf[channel].imax != atoi(s_imax) ) {
				ainputs_conf[channel].imax = atoi(s_imax);
			}
		}

		if ( s_offset != NULL ) {
			if ( ainputs_conf[channel].offset != atof(s_offset) ) {
				ainputs_conf[channel].offset = atof(s_offset);
			}
		}

		if ( s_mmin != NULL ) {
			ainputs_conf[channel].mmin = atof(s_mmin);
		}

		if ( s_mmax != NULL ) {
			ainputs_conf[channel].mmax = atof(s_mmax);
		}

		retS = true;
	}

	return(retS);
}
//------------------------------------------------------------------------------
void ainputs_config_defaults(ainputs_conf_t *ainputs_conf)
{
    	/*
         * Realiza la configuracion por defecto de los canales digitales.
         */

uint8_t channel = 0;

	for ( channel = 0; channel < NRO_ANALOG_CHANNELS; channel++) {
		ainputs_conf[channel].imin = 0;
		ainputs_conf[channel].imax = 20;
		ainputs_conf[channel].mmin = 0.0;
		ainputs_conf[channel].mmax = 10.0;
		ainputs_conf[channel].offset = 0.0;

		snprintf_P( ainputs_conf[channel].name, AIN_PARAMNAME_LENGTH, PSTR("A%d\0"),channel );
	}

    snprintf_P( ainputs_conf[2].name, AIN_PARAMNAME_LENGTH, PSTR("X"));

}
//------------------------------------------------------------------------------
void ainputs_print_configuration(ainputs_conf_t *ainputs_conf)
{
    /*
     * Muestra la configuracion de todos los canales analogicos en la terminal
     * La usa el comando tkCmd::status.
     */
    
uint8_t channel = 0;

    xprintf_P(PSTR("Ainputs:\r\n"));
    xprintf_P(PSTR(" debug: "));
    f_debug_ainputs ? xprintf_P(PSTR("true\r\n")) : xprintf_P(PSTR("false\r\n"));

	for ( channel = 0; channel < NRO_ANALOG_CHANNELS; channel++) {
		xprintf_P( PSTR(" a%d: [%d-%d mA/ %.02f,%.02f | %.03f | %s]\r\n"),
			channel,
			ainputs_conf[channel].imin,
			ainputs_conf[channel].imax,
			ainputs_conf[channel].mmin,
			ainputs_conf[channel].mmax,
			ainputs_conf[channel].offset,
			ainputs_conf[channel].name );
	}

}
//------------------------------------------------------------------------------
uint16_t ainputs_read_channel_raw(uint8_t channel )
{

        /*
         * Lee el valor del INA del canal analogico dado
         */
    
uint8_t ina_reg = 0;
uint16_t an_raw_val = 0;
uint8_t MSB = 0;
uint8_t LSB = 0;
char res[3] = { '\0','\0', '\0' };
int8_t xBytes = 0;
//float vshunt;

	switch ( channel ) {
	case 0:
		ina_reg = INA3221_CH3_SHV;
		break;
	case 1:
		ina_reg = INA3221_CH2_SHV;
		break;
	case 2:
		ina_reg = INA3221_CH1_SHV;
		break;
    case 99:
         // Battery ??
        ina_reg = INA3221_CH1_BUSV;
        break;
	default:
		return(-1);
		break;
	}

	// Leo el valor del INA.
	xBytes = INA_read( ina_reg, res ,2 );

	if ( xBytes == -1 )
		xprintf_P(PSTR("ERROR I2C: ainputs_read_channel_raw.\r\n\0"));

	an_raw_val = 0;
	MSB = res[0];
	LSB = res[1];
	an_raw_val = ( MSB << 8 ) + LSB;
	an_raw_val = an_raw_val >> 3;

    if ( f_debug_ainputs ) {
        xprintf_P( PSTR("INA: ch=%d, reg=%d, MSB=0x%x, LSB=0x%x, ANV=(0x%x)%d\r\n") ,channel, ina_reg, MSB, LSB, an_raw_val, an_raw_val );
    }
    
//	vshunt = (float) an_raw_val * 40 / 1000;
//	xprintf_P( PSTR("out->ACH: ch=%d, ina=%d, reg=%d, MSB=0x%x, LSB=0x%x, ANV=(0x%x)%d, VSHUNT = %.02f(mV)\r\n\0") ,channel_id, ina_id, ina_reg, MSB, LSB, an_raw_val, an_raw_val, vshunt );

	return( an_raw_val );
}
//------------------------------------------------------------------------------
float ainputs_read_channel_mag(uint8_t channel, ainputs_conf_t *ainputs_conf, uint16_t an_raw_val)
{
    /*
     * Convierte el valor raw a la magnitud.
     */
    
float an_mag_val = 0.0;
float I = 0.0;
float M = 0.0;
float P = 0.0;
uint16_t D = 0;

    // Battery ??
    if ( channel == 99 ) {
        // Convierto el raw_value a la magnitud ( 8mV por count del A/D)
        an_mag_val =  0.008 * an_raw_val;
        if ( f_debug_ainputs ) {
            xprintf_P(PSTR("ANALOG: A%d (RAW=%d), BATT=%.03f\r\n\0"), channel, an_raw_val, an_mag_val );
        }
        goto quit;
    }
    
    
	// Convierto el raw_value a corriente
	I = (float) an_raw_val / INA_FACTOR;
    
	// Calculo la magnitud
	P = 0;
	D = ainputs_conf[channel].imax - ainputs_conf[channel].imin;
	an_mag_val = 0.0;
	if ( D != 0 ) {
		// Pendiente
		P = (float) ( ainputs_conf[channel].mmax  -  ainputs_conf[channel].mmin ) / D;
		// Magnitud
		M = (float) ( ainputs_conf[channel].mmin + ( I - ainputs_conf[channel].imin ) * P);

		// Al calcular la magnitud, al final le sumo el offset.
		an_mag_val = M + ainputs_conf[channel].offset;
		// Corrijo el 0 porque sino al imprimirlo con 2 digitos puede dar negativo
		if ( fabs(an_mag_val) < 0.01 )
			an_mag_val = 0.0;

	} else {
		// Error: denominador = 0.
		an_mag_val = -999.0;
	}

    if ( f_debug_ainputs ) {
        xprintf_P(PSTR("ANALOG: A%d (RAW=%d), I=%.03f\r\n\0"), channel, an_raw_val, I );
        xprintf_P(PSTR("ANALOG: Imin=%d, Imax=%d\r\n\0"), ainputs_conf[channel].imin, ainputs_conf[channel].imax );
        xprintf_P(PSTR("ANALOG: mmin=%.03f, mmax=%.03f\r\n\0"), ainputs_conf[channel].mmin, ainputs_conf[channel].mmax );
        xprintf_P(PSTR("ANALOG: D=%d, P=%.03f, M=%.03f\r\n\0"), D, P, M );
        xprintf_P(PSTR("ANALOG: an_raw_val=%d, an_mag_val=%.03f\r\n\0"), an_raw_val, an_mag_val );
    }

quit:
        
	return(an_mag_val);
}
//------------------------------------------------------------------------------
void ainputs_read_channel ( uint8_t channel, ainputs_conf_t *ainputs_conf, float *mag, uint16_t *raw )
{
	/*
	Lee un canal analogico y devuelve el valor convertido a la magnitud configurada.
	Es publico porque se utiliza tanto desde el modo comando como desde el modulo de poleo de las entradas.
	Hay que corregir la correspondencia entre el canal leido del INA y el canal fisico del datalogger
	io_channel. Esto lo hacemos en AINPUTS_read_ina.

	la funcion read_channel_raw me devuelve el valor raw del conversor A/D.
	Este corresponde a 40uV por bit por lo tanto multiplico el valor raw por 40/1000 y obtengo el valor en mV.
	Como la resistencia es de 7.32, al dividirla en 7.32 tengo la corriente medida.
	Para pasar del valor raw a la corriente debo hacer:
	- Pasar de raw a voltaje: V = raw * 40 / 1000 ( en mV)
	- Pasar a corriente: I = V / 7.32 ( en mA)
	- En un solo paso haria: I = raw / 3660
	  3660 = 40 / 1000 / 7.32.
	  Este valor 3660 lo llamamos INASPAN y es el valor por el que debo multiplicar el valor raw para que con una
	  resistencia shunt de 7.32 tenga el valor de la corriente medida. !!!!
	*/


uint16_t an_raw_val = 0;
float an_mag_val = 0.0;
t_ain_s rb_element;
float avg;
uint8_t i;

	// Leo el valor del INA.(raw)
	an_raw_val = ainputs_read_channel_raw( channel );
    // Lo convierto a la magnitud
    an_mag_val = ainputs_read_channel_mag( channel, ainputs_conf, an_raw_val);
    
    // Lo almaceno en el RB correspondiente
    rb_element.ain = an_mag_val;
    switch(channel) {
        case 0:
            rBstruct_Poke(&ain_RB_0, &rb_element);
            break;
        case 1:
            rBstruct_Poke(&ain_RB_1, &rb_element);
            break;
        case 2:
            rBstruct_Poke(&ain_RB_2, &rb_element);
            break;
        case 99:
            rBstruct_Poke(&ain_RB_2, &rb_element);
            break;
            
    }
    // Promedio los resultados del RB y devuelvo este dato
    avg=0.0;
    for(i=0; i < max_rb_ain_storage_size; i++) {
        switch(channel) {
            case 0:
                rb_element = ain_storage_0[i];    
                break;
            case 1:
                rb_element = ain_storage_1[i];    
                break;
            case 2:
                rb_element = ain_storage_2[i];    
                break;     
        }
        avg += rb_element.ain;
        if ( f_debug_ainputs ) {
            xprintf_P(PSTR("DEBUG AIN%02d: %d,value=%0.3f,avg=%0.3f\r\n"), channel, i, rb_element.ain, avg );
        }
    }
    avg /= max_rb_ain_storage_size;
    
	*raw = an_raw_val;
	//*mag = an_mag_val;
    *mag = avg;

}
//------------------------------------------------------------------------------
void ainputs_prender_sensores(void)
{
    /* 
     * Para ahorrar energia los canales se prenden cuando se necesitan y luego
     * se apagan
     */

uint32_t sleep_time_ms;

    ainputs_awake();
	//
	if ( ! sensores_prendidos ) {
		SET_VSENSORS420();
		sensores_prendidos = true;
		// Normalmente espero 1s de settle time que esta bien para los sensores
		// pero cuando hay un caudalimetro de corriente, necesita casi 5s
		// vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
        sleep_time_ms = (uint32_t)PWRSENSORES_SETTLETIME_MS / portTICK_PERIOD_MS; 
		vTaskDelay( ( TickType_t)sleep_time_ms );
	}
}
//------------------------------------------------------------------------------
void ainputs_apagar_sensores(void)
{

	CLEAR_VSENSORS420();
	sensores_prendidos = false;
	ainputs_sleep();

}
//------------------------------------------------------------------------------
bool ainputs_test_read_channel( uint8_t channel, ainputs_conf_t *ainputs_conf )
{
  
float mag;
uint16_t raw;

    if ( ( channel == 0 ) || (channel == 1 ) || ( channel == 2) || ( channel == 99)) {
        ainputs_prender_sensores();
        ainputs_read_channel ( channel, ainputs_conf, &mag, &raw );
        xprintf_P(PSTR("AINPUT ch%d=%0.3f\r\n"), channel, mag);
        ainputs_apagar_sensores();
        return(true);
    } else {
        return(false);
    }

}
//------------------------------------------------------------------------------
void ainputs_config_debug(bool debug )
{
    if ( debug ) {
        f_debug_ainputs = true;
    } else {
        f_debug_ainputs = false;
    }
    
}
//------------------------------------------------------------------------------
bool ainputs_read_debug(void)
{
    return (f_debug_ainputs);
}
//------------------------------------------------------------------------------
