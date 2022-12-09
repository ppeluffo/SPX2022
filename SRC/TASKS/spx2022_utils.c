/* 
 * File:   frtos20_utils.c
 * Author: pablo
 *
 * Created on 22 de diciembre de 2021, 07:34 AM
 */

#include "spx2022.h"

const uint8_t hash_table[] PROGMEM =  {
		 93,  153, 124,  98, 233, 146, 184, 207, 215,  54, 208, 223, 254, 216, 162, 141,
		 10,  148, 232, 115,   7, 202,  66,  31,   1,  33,  51, 145, 198, 181,  13,  95,
		 242, 110, 107, 231, 140, 170,  44, 176, 166,   8,   9, 163, 150, 105, 113, 149,
		 171, 152,  58, 133, 186,  27,  53, 111, 210,  96,  35, 240,  36, 168,  67, 213,
		 12,  123, 101, 227, 182, 156, 190, 205, 218, 139,  68, 217,  79,  16, 196, 246,
		 154, 116,  29, 131, 197, 117, 127,  76,  92,  14,  38,  99,   2, 219, 192, 102,
		 252,  74,  91, 179,  71, 155,  84, 250, 200, 121, 159,  78,  69,  11,  63,   5,
		 126, 157, 120, 136, 185,  88, 187, 114, 100, 214, 104, 226,  40, 191, 194,  50,
		 221, 224, 128, 172, 135, 238,  25, 212,   0, 220, 251, 142, 211, 244, 229, 230,
		 46,   89, 158, 253, 249,  81, 164, 234, 103,  59,  86, 134,  60, 193, 109,  77,
		 180, 161, 119, 118, 195,  82,  49,  20, 255,  90,  26, 222,  39,  75, 243, 237,
		 17,   72,  48, 239,  70,  19,   3,  65, 206,  32, 129,  57,  62,  21,  34, 112,
		 4,    56, 189,  83, 228, 106,  61,   6,  24, 165, 201, 167, 132,  45, 241, 247,
		 97,   30, 188, 177, 125,  42,  18, 178,  85, 137,  41, 173,  43, 174,  73, 130,
		 203, 236, 209, 235,  15,  52,  47,  37,  22, 199, 245,  23, 144, 147, 138,  28,
		 183,  87, 248, 160,  55,  64, 204,  94, 225, 143, 175, 169,  80, 151, 108, 122
};

//------------------------------------------------------------------------------
int8_t WDT_init(void);
int8_t CLKCTRL_init(void);
uint8_t checksum( uint8_t *s, uint16_t size );

//-----------------------------------------------------------------------------
void system_init()
{

	CLKCTRL_init();
    WDT_init();
    LED_init();
    XPRINTF_init();
    //COUNTERS_init();
    OCOUT_init();
    VSENSORS420_init();
    CONFIG_RTS_485A();
    CONFIG_RTS_485B();
}
//-----------------------------------------------------------------------------
int8_t WDT_init(void)
{
	/* 8K cycles (8.2s) */
	/* Off */
	ccp_write_io((void *)&(WDT.CTRLA), WDT_PERIOD_8KCLK_gc | WDT_WINDOW_OFF_gc );  
	return 0;
}
//-----------------------------------------------------------------------------
int8_t CLKCTRL_init(void)
{
	// Configuro el clock para 24Mhz
	
	ccp_write_io((void *)&(CLKCTRL.OSCHFCTRLA), CLKCTRL_FREQSEL_24M_gc         /* 24 */
	| 0 << CLKCTRL_AUTOTUNE_bp /* Auto-Tune enable: disabled */
	| 0 << CLKCTRL_RUNSTDBY_bp /* Run standby: disabled */);

	// ccp_write_io((void*)&(CLKCTRL.MCLKCTRLA),CLKCTRL_CLKSEL_OSCHF_gc /* Internal high-frequency oscillator */
	//		 | 0 << CLKCTRL_CLKOUT_bp /* System clock out: disabled */);

	// ccp_write_io((void*)&(CLKCTRL.MCLKLOCK),0 << CLKCTRL_LOCKEN_bp /* lock enable: disabled */);

	return 0;
}
//-----------------------------------------------------------------------------
void reset(void)
{
	/* Issue a Software Reset to initilize the CPU */
	ccp_write_io( (void *)&(RSTCTRL.SWRR), RSTCTRL_SWRST_bm ); 
                                           
}
//------------------------------------------------------------------------------
void kick_wdt( uint8_t bit_pos)
{
    // Pone el bit correspondiente en 0.
    sys_watchdog &= ~ (1 << bit_pos);
    
}
//------------------------------------------------------------------------------
void config_default(void)
{

    systemConf.wan_port = WAN_RS485B;
    memcpy(systemConf.dlgid, "DEFAULT\0", sizeof(systemConf.dlgid));
    systemConf.timerpoll = 30;
    ainputs_config_defaults(systemConf.ainputs_conf);
    counters_config_defaults(systemConf.counters_conf);
  
}
//------------------------------------------------------------------------------
bool config_debug( char *tipo, char *valor)
{
    /*
     * Configura las flags de debug para ayudar a visualizar los problemas
     */
    
    if (!strcmp_P( strupr(tipo), PSTR("NONE")) ) {
        ainputs_config_debug(false);
        counters_config_debug(false);
        return(true); 
    }
    
    if (!strcmp_P( strupr(tipo), PSTR("ANALOG")) ) {
        if (!strcmp_P( strupr(valor), PSTR("TRUE")) ) {
            ainputs_config_debug(true);
            return(true);
        }
        if (!strcmp_P( strupr(valor), PSTR("FALSE")) ) {
            ainputs_config_debug(false);
            return(true);
        }
    }

    if (!strcmp_P( strupr(tipo), PSTR("COUNTERS")) ) {
        if (!strcmp_P( strupr(valor), PSTR("TRUE")) ) {
            counters_config_debug(true);
            return(true);
        }
        if (!strcmp_P( strupr(valor), PSTR("FALSE")) ) {
            counters_config_debug(false);
            return(true);
        }
    }
    
    if (!strcmp_P( strupr(tipo), PSTR("COMMS")) ) {
        if (!strcmp_P( strupr(valor), PSTR("TRUE")) ) {
            WAN_config_debug(true);
            return(true);
        }
        if (!strcmp_P( strupr(valor), PSTR("FALSE")) ) {
            WAN_config_debug(false);
            return(true);
        }
    }
    
    return(false);
    
}
//------------------------------------------------------------------------------
bool save_config_in_NVM(void)
{
   
int8_t retVal;
uint8_t cks;

    cks = checksum ( (uint8_t *)&systemConf, ( sizeof(systemConf) - 1));
    systemConf.checksum = cks;
    
    retVal = NVMEE_write( 0x00, (char *)&systemConf, sizeof(systemConf) );
    if (retVal == -1 )
        return(false);
    
    return(true);
   
}
//------------------------------------------------------------------------------
bool load_config_from_NVM(void)
{

uint8_t rd_cks, calc_cks;
    
    NVMEE_read( 0x00, (char *)&systemConf, sizeof(systemConf) );
    rd_cks = systemConf.checksum;
    
    calc_cks = checksum ( (uint8_t *)&systemConf, ( sizeof(systemConf) - 1));
    
    if ( calc_cks != rd_cks ) {
		xprintf_P( PSTR("ERROR: Checksum systemVars failed: calc[0x%0x], read[0x%0x]\r\n"), calc_cks, rd_cks );
        
		return(false);
	}
    
    return(true);
}
//------------------------------------------------------------------------------
uint8_t checksum( uint8_t *s, uint16_t size )
{
	/*
	 * Recibe un puntero a una estructura y un tamaño.
	 * Recorre la estructura en forma lineal y calcula el checksum
	 */

uint8_t *p = NULL;
uint8_t cks = 0;
uint16_t i = 0;

	cks = 0;
	p = s;
	for ( i = 0; i < size ; i++) {
		 cks = (cks + (int)(p[i])) % 256;
	}

	return(cks);
}
//------------------------------------------------------------------------------
bool config_wan_port(char *comms_type)
{
    if ((strcmp_P( strupr(comms_type), PSTR("RS485")) == 0) ) {
        systemConf.wan_port = WAN_RS485B;
        return(true);
    }
    
    if ((strcmp_P( strupr(comms_type), PSTR("NBIOT")) == 0) ) {
        systemConf.wan_port = WAN_NBIOT;
        return(true);
    }
    return(false);
    
}
//------------------------------------------------------------------------------
uint8_t u_hash(uint8_t checksum, char ch )
{
	/*
	 * Funcion de hash de pearson modificada.
	 * https://es.qwe.wiki/wiki/Pearson_hashing
	 * La función original usa una tabla de nros.aleatorios de 256 elementos.
	 * Ya que son aleatorios, yo la modifico a algo mas simple.
	 */

uint8_t h_entry = 0;
uint8_t h_val = 0;
uint8_t ord;

	ord = (int)ch;
	h_entry = checksum ^ ord;
	h_val = (PGM_P)pgm_read_byte_far( &(hash_table[h_entry]));
	return(h_val);

}
//------------------------------------------------------------------------------
void data_resync_clock( RtcTimeType_t *rtc_s, bool force_adjust)
{
	/*
	 * Ajusta el clock interno de acuerdo al valor de rtc_s
	 * Bug 01: 2021-12-14:
	 * El ajuste no considera los segundos entonces si el timerpoll es c/15s, cada 15s
	 * se reajusta y cambia la hora del datalogger.
	 * Modifico para que el reajuste se haga si hay una diferencia de mas de 90s entre
	 * el reloj local y el del server
	 */


float diff_seconds;
RtcTimeType_t rtc_l;
int8_t xBytes = 0;
char rtcStr[12];

    // Convierto el rtc a formato YYMMDDHHMM
    RTC_rtc2str(rtcStr, rtc_s, true );
        
	if ( force_adjust ) {
		// Fuerzo el ajuste.( al comienzo )
		xBytes = RTC_write_dtime( rtc_s);		// Grabo el RTC
		if ( xBytes == -1 ) {
			xprintf_P(PSTR("ERROR: CLOCK: I2C:RTC:pv_process_server_clock\r\n\0"));
		} else {
			xprintf_P( PSTR("CLOCK: Update rtc to: %s\r\n\0"), rtcStr );
		}
		return;
	}

	// Solo ajusto si la diferencia es mayor de 90s
	// Veo la diferencia de segundos entre ambos.
	// Asumo yy,mm,dd iguales
	// Leo la hora actual del datalogger
	RTC_read_dtime( &rtc_l);
	diff_seconds = abs( rtc_l.hour * 3600 + rtc_l.min * 60 + rtc_l.sec - ( rtc_s->hour * 3600 + rtc_s->min * 60 + rtc_s->sec));
	//xprintf_P( PSTR("COMMS: rtc diff=%.01f\r\n"), diff_seconds );

	if ( diff_seconds > 90 ) {
		// Ajusto
		xBytes = RTC_write_dtime(rtc_s);		// Grabo el RTC
		if ( xBytes == -1 ) {
			xprintf_P(PSTR("ERROR: CLOCK: I2C:RTC:pv_process_server_clock\r\n\0"));
		} else {
			xprintf_P( PSTR("CLOCK: Update rtc to: %s\r\n\0"), rtcStr );
		}
		return;
	}
}
//------------------------------------------------------------------------------