
#include "spx2022.h"
#include "frtos_cmd.h"


static void cmdClsFunction(void);
static void cmdHelpFunction(void);
static void cmdResetFunction(void);
static void cmdStatusFunction(void);
static void cmdWriteFunction(void);
static void cmdReadFunction(void);
static void cmdConfigFunction(void);
static void cmdTestFunction(void);

static void pv_snprintfP_OK(void );
static void pv_snprintfP_ERR(void );

char rw_buffer[FF_RECD_SIZE];

//------------------------------------------------------------------------------
void tkCmd(void * pvParameters)
{

	// Esta es la primer tarea que arranca.

( void ) pvParameters;

    while ( ! starting_flag )
        vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

	//vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );

uint8_t c = 0;
uint16_t sleep_timeout;

    FRTOS_CMD_init();

    FRTOS_CMD_register( "cls", cmdClsFunction );
	FRTOS_CMD_register( "help", cmdHelpFunction );
    FRTOS_CMD_register( "reset", cmdResetFunction );
    FRTOS_CMD_register( "status", cmdStatusFunction );
    FRTOS_CMD_register( "write", cmdWriteFunction );
    FRTOS_CMD_register( "read", cmdReadFunction );
    FRTOS_CMD_register( "config", cmdConfigFunction );
    FRTOS_CMD_register( "test", cmdTestFunction );
    
    xprintf_P(PSTR("Starting tkCmd..\r\n" ));
    xprintf_P(PSTR("Spymovil %s %s %s %s \r\n") , HW_MODELO, FRTOS_VERSION, FW_REV, FW_DATE);
    
    sleep_timeout = 1500; // Espero hasta 30s secs
    
	// loop
	for( ;; )
	{
        kick_wdt(CMD_WDG_bp);
         
		c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.
		// el read se bloquea 10ms. lo que genera la espera.
		//while ( frtos_read( fdTERM, (char *)&c, 1 ) == 1 ) {
        while ( xgetc( (char *)&c ) == 1 ) {
            FRTOS_CMD_process(c);
            sleep_timeout = 1500;
        }
        
        // Espero 10ms si no hay caracteres en el buffer
        vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );
        
        // Luego de 30 secs de inactividad, duermo 20 secs
        /*
        if ( sleep_timeout-- == 0 ) {
            //xprintf_P(PSTR("Going to sleep\r\n"));
            vTaskDelay( ( TickType_t)( 20000 / portTICK_PERIOD_MS ) );
            sleep_timeout = 1;
        }
         */ 
        
	}    
}
//------------------------------------------------------------------------------
static void cmdTestFunction(void)
{

    FRTOS_CMD_makeArgv();

dataRcd_s dr;

    if (!strcmp_P( strupr(argv[1]), PSTR("FSDEBUGON"))  ) {
        FS_set_debug();
        return;
    }

    if (!strcmp_P( strupr(argv[1]), PSTR("FSDEBUGOFF"))  ) {
        FS_clear_debug();
        return;
    }

    if (!strcmp_P( strupr(argv[1]), PSTR("KILL"))  ) {
        if (!strcmp_P( strupr(argv[2]), PSTR("WAN"))  ) {  
            if ( xHandle_tkWAN != NULL ) {
                vTaskSuspend( xHandle_tkWAN );
                xHandle_tkWAN = NULL;
            }
            return;
        }
        
        if (!strcmp_P( strupr(argv[2]), PSTR("SYS"))  ) {
            if ( xHandle_tkSys != NULL ) {
                vTaskSuspend( xHandle_tkSys );
                xHandle_tkSys = NULL;
            }
            return;
        }        
        xprintf_P(PSTR("test kill {sys,wan}\r\n"));
        return;
    }

    if (!strcmp_P( strupr(argv[1]), PSTR("POLL"))  ) {
        poll_data(&dr);
        WAN_process_data_rcd(&dr);
        xprint_dr(&dr);
        return;
    }

    if (!strcmp_P( strupr(argv[1]), PSTR("READRCD"))  ) {
        FS_readRcdByPos( atoi(argv[2]), &dr, sizeof(dataRcd_s), false );
        xprint_dr(&dr);
        return;
    }

    if (!strcmp_P( strupr(argv[1]), PSTR("READDRCD"))  ) {
        FS_readRcdByPos( atoi(argv[2]), &dr, sizeof(dataRcd_s), true );
        xprint_dr(&dr);
        return;
    }

    if (!strcmp_P( strupr(argv[1]), PSTR("WRITE\0"))  ) {
		xprintf_P(PSTR("Testing write FF\r\n"));
        dr.l_ainputs[0] = 10.32;
        dr.l_ainputs[1] = 11.42;
        dr.l_ainputs[2] = 110.01;
        dr.l_counters[0] = 221.0;
        dr.l_counters[1] = 32.64;
        dr.rtc.year = 22;
        dr.rtc.month = 12;
        dr.rtc.day = 18;
        dr.rtc.hour = 12;
        dr.rtc.min = 30;
        dr.rtc.sec = 00;
        FS_writeRcd( &dr, sizeof(dataRcd_s) );
		return;
	}

    xprintf_P( PSTR("Test: poll, readrcd {pos},readdrcd {pos}, fsdebugon, fsdebugoff, write, kill {wan,sys}\r\n"));
    return;
    
/*
int8_t i;
char c;
float f;
char s1[20];

	xprintf("Test function\r\n");
    i=10;
    xprintf("Integer: %d\r\n", i);
    c='P';
    xprintf("Char: %c\r\n", c);
    f=12.32;
    xprintf("FLoat: %0.3f\r\n", f);
    
    strncpy(s1,"Pablo Peluffo", 20);
    xprintf("String: %s\r\n", s1);
    // 
    xprintf("Todo junto: [d]=%d, [c]=%c, [s]=%s, [f]=%0.3f\r\n",i,c,s1,f);
    
    // STRINGS IN ROM
    
    xprintf_P(PSTR("Strings in ROM\r\n"));
    i=11;
    xprintf_P(PSTR("Integer: %d\r\n"), i);
    c='Q';
    xprintf_P(PSTR("Char: %c\r\n"), c);

    f=15.563;
    xprintf_P(PSTR("FLoat: %0.3f\r\n"), f);
    strncpy(s1,"Keynet Spymovil", 20);
    xprintf_P(PSTR("String: %s\r\n"), s1);
    // 
    xprintf_P(PSTR("Todo junto: [d]=%d, [c]=%c, [s]=%s, [f]=%0.3f\r\n"),i,c,s1,f);
   
    // DEFINED
    xprintf("Spymovil %s %s %s %s \r\n" , HW_MODELO, FRTOS_VERSION, FW_REV, FW_DATE);
*/
    
}
//------------------------------------------------------------------------------
static void cmdHelpFunction(void)
{

    FRTOS_CMD_makeArgv();
        
    if ( !strcmp_P( strupr(argv[1]), PSTR("WRITE"))) {
		xprintf_P( PSTR("-write:\r\n"));
        xprintf_P( PSTR("  (ee,nvmee,rtcram) {pos string} {debug}\r\n"));
        xprintf_P( PSTR("  rtc YYMMDDhhmm\r\n"));
        xprintf_P( PSTR("  rele {on/off}, vsensors420 {on/off}\r\n"));
        xprintf_P( PSTR("  ina {confValue}\r\n"));
        xprintf_P( PSTR("  rs485a {string}, rs485b {string}\r\n"));
        
    }  else if ( !strcmp_P( strupr(argv[1]), PSTR("READ"))) {
		xprintf_P( PSTR("-read:\r\n"));
        xprintf_P( PSTR("  (ee,nvmee,rtcram) {pos} {lenght} {debug}\r\n"));
        xprintf_P( PSTR("  avrid,rtc {long,short}\r\n"));
        xprintf_P( PSTR("  cnt {0,1}\r\n"));
        xprintf_P( PSTR("  ina {conf|chXshv|chXbusv|mfid|dieid}\r\n"));
        xprintf_P( PSTR("  rs485a, rs485b\r\n"));
        xprintf_P( PSTR("  ainput {n}\r\n"));
        xprintf_P( PSTR("  memory {full}\r\n"));
        
    }  else if ( !strcmp_P( strupr(argv[1]), PSTR("CONFIG"))) {
		xprintf_P( PSTR("-config:\r\n"));
        xprintf_P( PSTR("  dlgid\r\n"));
        xprintf_P( PSTR("  default\r\n"));
        xprintf_P( PSTR("  save\r\n"));
        xprintf_P( PSTR("  comms {rs485, nbiot}, timerpoll, timerdial\r\n"));
        xprintf_P( PSTR("  pwrmodo {continuo,discreto,mixto}, pwron {hhmm}, pwroff {hhmm}\r\n"));
        xprintf_P( PSTR("  ainput {0..%d} aname imin imax mmin mmax offset\r\n"),( NRO_ANALOG_CHANNELS - 1 ) );
        xprintf_P( PSTR("  counter {0..%d} cname magPP modo(PULSO/CAUDAL)\r\n"), ( NRO_COUNTER_CHANNELS - 1 ) );
        xprintf_P( PSTR("  debug {analog,counters,comms,none} {true/false}\r\n"));
        
    	// HELP RESET
	} else if (!strcmp_P( strupr(argv[1]), PSTR("RESET"))) {
		xprintf_P( PSTR("-reset\r\n"));
        xprintf_P( PSTR("  memory {soft|hard}\r\n\0"));
		return;

    }  else {
        // HELP GENERAL
        xprintf("Available commands are:\r\n");
        xprintf("-cls\r\n");
        xprintf("-help\r\n");
        xprintf("-status\r\n");
        xprintf("-reset\r\n");
        xprintf("-write...\r\n");
        xprintf("-config...\r\n");
        xprintf("-read...\r\n");

    }
   
	xprintf("Exit help \r\n");

}
//------------------------------------------------------------------------------
static void cmdReadFunction(void)
{
    
    FRTOS_CMD_makeArgv();

    // MEMORY
	// read memory
	if (!strcmp_P( strupr(argv[1]), PSTR("MEMORY\0"))  ) {
		FS_dump(xprint_from_dump);
		return;
	}
    
    // AINPUT
    // read ainput {n}
	if (!strcmp_P( strupr(argv[1]), PSTR("AINPUT"))  ) {
        ainputs_test_read_channel( atoi(argv[2]), systemConf.ainputs_conf ) ? pv_snprintfP_OK(): pv_snprintfP_ERR();
		return;
	}
    
    // INA
	// read ina regName
	if (!strcmp_P( strupr(argv[1]), PSTR("INA"))  ) {
		INA_awake();
		INA_test_read ( argv[2] );
		INA_sleep();
		return;
	}
    
    // CNT{0,1}
	// read cnt
	if (!strcmp_P( strupr(argv[1]), PSTR("CNT")) ) {
        if ( atoi(argv[2]) == 0 ) {
            xprintf_P(PSTR("CNT0=%d\r\n"), CNT0_read());
            pv_snprintfP_OK();
            return;
        }
        if ( atoi(argv[2]) == 1 ) {
            xprintf_P(PSTR("CNT1=%d\r\n"), CNT1_read());
            pv_snprintfP_OK();
            return;
        }
        pv_snprintfP_ERR();
        return;
	}
    
    // EE
	// read ee address length
	if (!strcmp_P( strupr(argv[1]), PSTR("EE")) ) {
		EE_test_read ( argv[2], argv[3], argv[4] );
		return;
	}
    
    // RTC
	// read rtc { long | short }
    if (!strcmp_P( strupr(argv[1]), PSTR("RTC")) ) {
        if (!strcmp_P( strupr(argv[2]), PSTR("LONG")) ) {
            RTC_read_time(FORMAT_LONG);
            pv_snprintfP_OK();
            return;
        }
        if (!strcmp_P( strupr(argv[2]), PSTR("SHORT")) ) {
            RTC_read_time(FORMAT_SHORT);
            pv_snprintfP_OK();
            return;
        }
        pv_snprintfP_ERR();
        return;
    }
        
    
    // NVMEE
	// read nvmee address length
	if (!strcmp_P( strupr(argv[1]), PSTR("NVMEE")) ) {
		NVMEE_test_read ( argv[2], argv[3] );
		return;
	}

	// RTC SRAM
	// read rtcram address length
	if (!strcmp_P( strupr(argv[1]), PSTR("RTCRAM"))) {
		RTCSRAM_test_read ( argv[2], argv[3] );
		return;
	}

	// AVRID
	// read avrid
	if (!strcmp_P( strupr(argv[1]), PSTR("AVRID"))) {
		//nvm_read_print_id();
        xprintf_P(PSTR("ID: %s\r\n"), NVM_id2str() );
        xprintf_P(PSTR("SIGNATURE: %s\r\n"), NVM_signature2str() );
		return;
	}
    
    
    // CMD NOT FOUND
	xprintf("ERROR\r\nCMD NOT DEFINED\r\n\0");
	return;
 
}
//------------------------------------------------------------------------------
static void cmdClsFunction(void)
{
	// ESC [ 2 J
	xprintf("\x1B[2J\0");
}
//------------------------------------------------------------------------------
static void cmdResetFunction(void)
{
    
    FRTOS_CMD_makeArgv();
    
    // Reset memory ??
    if (!strcmp_P( strupr(argv[1]), PSTR("MEMORY"))) {
        
        /*
         * No puedo estar usando la memoria !!!
         */
        if ( xHandle_tkSys != NULL ) {
            vTaskSuspend( xHandle_tkSys );
            xHandle_tkSys = NULL;
        }
        
        if ( xHandle_tkWAN != NULL ) {
            vTaskSuspend( xHandle_tkWAN );
            xHandle_tkWAN = NULL;
        }
        
        if ( !strcmp_P( strupr(argv[2]), PSTR("SOFT"))) {
			FS_format(false );
		} else if ( !strcmp_P( strupr(argv[2]), PSTR("HARD"))) {
			FS_format(true);
		} else {
			xprintf_P( PSTR("ERROR\r\nUSO: reset memory {hard|soft}\r\n"));
			return;
		}
    }
    
    xprintf("Reset..\r\n");
    reset();
}
//------------------------------------------------------------------------------
static void cmdStatusFunction(void)
{

    // https://stackoverflow.com/questions/12844117/printing-defined-constants

fat_s l_fat;

    xprintf("Spymovil %s %s TYPE=%s, VER=%s %s \r\n" , HW_MODELO, FRTOS_VERSION, FW_TYPE, FW_REV, FW_DATE);
     
    // Memoria
    FAT_read(&l_fat);
	xprintf_P( PSTR("memory: rcdSize=%d, size=%d, wrPtr=%d, rdPtr=%d,count=%d\r\n"),sizeof(dataRcd_s), FF_MAX_RCDS, l_fat.head,l_fat.tail, l_fat.count );
 
    xprintf_P(PSTR("Config:\r\n"));
    xprintf_P(PSTR(" date: %s\r\n"), RTC_logprint(FORMAT_LONG));
    xprintf_P(PSTR(" dlgid: %s\r\n"), systemConf.dlgid );
    xprintf_P(PSTR(" timerpoll=%d\r\n"), systemConf.timerpoll);
    xprintf_P(PSTR(" timerdial=%d\r\n"), systemConf.timerdial);
    
    print_pwr_configuration();
    
    WAN_print_configuration();
    ainputs_print_configuration( systemConf.ainputs_conf);
    counters_config_print( systemConf.counters_conf);
    
    xprintf_P(PSTR("Values:\r\n"));
    xprintf_P(PSTR(" Rele: %d\r\n"), systemVars.rele_output);
    xprintf_P(PSTR(" Frame: "));
    xprint_dr( get_system_dr());
    
}
//------------------------------------------------------------------------------
static void cmdWriteFunction(void)
{

    FRTOS_CMD_makeArgv(); 

    // RS485A
    // write rs485a {string}
    if ((strcmp_P( strupr(argv[1]), PSTR("RS485A")) == 0) ) {
        xfprintf_P( fdRS485A, PSTR("%s\r\n"), argv[2]);
        pv_snprintfP_OK();
        return;
    }

    // RS485B
    // write rs485b {string}
    if ((strcmp_P( strupr(argv[1]), PSTR("RS485B")) == 0) ) {
        xfprintf_P( fdRS485B, PSTR("%s\r\n"), argv[2]);
        pv_snprintfP_OK();
        return;
    }
    
    // INA
	// write ina rconfValue
	// Solo escribimos el registro 0 de configuracion.
	if ((strcmp_P( strupr(argv[1]), PSTR("INA")) == 0) ) {
        INA_awake();
		( INA_test_write ( argv[2] ) > 0)?  pv_snprintfP_OK() : pv_snprintfP_ERR();
        INA_sleep();
		return;
	}
    
    // write VSENSORS420 on/off
	if (!strcmp_P( strupr(argv[1]), PSTR("VSENSORS420")) ) {
        if (!strcmp_P( strupr(argv[2]), PSTR("ON")) ) {
            SET_VSENSORS420();
            pv_snprintfP_OK();
            return;
        }
        if (!strcmp_P( strupr(argv[2]), PSTR("OFF")) ) {
            CLEAR_VSENSORS420();
            pv_snprintfP_OK();
            return;
        }
        pv_snprintfP_ERR();
        return;
	}
    
	// write rele on/off
	if (!strcmp_P( strupr(argv[1]), PSTR("RELE")) ) {
        if (!strcmp_P( strupr(argv[2]), PSTR("ON")) ) {
            SET_RELEOUT();
            pv_snprintfP_OK();
            return;
        }
        if (!strcmp_P( strupr(argv[2]), PSTR("OFF")) ) {
            CLEAR_RELEOUT();
            pv_snprintfP_OK();
            return;
        }
        pv_snprintfP_ERR();
        return;
	}
    
   	// EE
	// write ee pos string
	if ((strcmp_P( strupr(argv[1]), PSTR("EE")) == 0) ) {
		( EE_test_write ( argv[2], argv[3], argv[4] ) > 0)?  pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}
    
    // RTC
	// write rtc YYMMDDhhmm
	if ( strcmp_P( strupr(argv[1]), PSTR("RTC")) == 0 ) {
		( RTC_write_time( argv[2]) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}
    
    // NVMEE
	// write nvmee pos string
	if ( (strcmp_P( strupr(argv[1]), PSTR("NVMEE")) == 0)) {
		NVMEE_test_write ( argv[2], argv[3] );
		pv_snprintfP_OK();
		return;
	}

	// RTC SRAM
	// write rtcram pos string
	if ( (strcmp_P( strupr(argv[1]), PSTR("RTCRAM")) == 0)  ) {
		( RTCSRAM_test_write ( argv[2], argv[3] ) > 0)?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}
    
   // CMD NOT FOUND
	xprintf("ERROR\r\nCMD NOT DEFINED\r\n\0");
	return;
 
}
//------------------------------------------------------------------------------
static void cmdConfigFunction(void)
{
  
bool retS = false;
    
    FRTOS_CMD_makeArgv();
     
    // POWER
    // pwr_modo {continuo,discreto,mixto}
    if (!strcmp_P( strupr(argv[1]), PSTR("PWRMODO\0"))) {
        config_pwrmodo(argv[2]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}
    
    // pwr_on {hhmm}
     if (!strcmp_P( strupr(argv[1]), PSTR("PWRON\0"))) {
        config_pwron(argv[2]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}  
    
    // pwr_off {hhmm}
     if (!strcmp_P( strupr(argv[1]), PSTR("PWROFF\0"))) {
        config_pwroff(argv[2]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}   
    
    // COMMS
    // comms {rs485, nbiot}
    if (!strcmp_P( strupr(argv[1]), PSTR("COMMS\0"))) {
        config_wan_port(argv[2]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}
        
    // DLGID
	if (!strcmp_P( strupr(argv[1]), PSTR("DLGID\0"))) {
		if ( argv[2] == NULL ) {
			retS = false;
			} else {
				memset(systemConf.dlgid,'\0', sizeof(systemConf.dlgid) );
				memcpy(systemConf.dlgid, argv[2], sizeof(systemConf.dlgid));
				systemConf.dlgid[DLGID_LENGTH - 1] = '\0';
				retS = true;
			}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}
    
    // DEFAULT
	// config default
	if (!strcmp_P( strupr(argv[1]), PSTR("DEFAULT\0"))) {
		config_default();
		pv_snprintfP_OK();
		return;
	}

	// SAVE
	// config save
	if (!strcmp_P( strupr(argv[1]), PSTR("SAVE\0"))) {
		save_config_in_NVM();
		pv_snprintfP_OK();
		return;
	}
    
    // LOAD
	// config load
	if (!strcmp_P( strupr(argv[1]), PSTR("LOAD\0"))) {
		load_config_from_NVM();
		pv_snprintfP_OK();
		return;
	}

    // TIMERPOLL
    // config timerpoll val
	if (!strcmp_P( strupr(argv[1]), PSTR("TIMERPOLL")) ) {
        config_timerpoll(argv[2]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}
    
    // TIMERDIAL
    // config timerdial val
	if (!strcmp_P( strupr(argv[1]), PSTR("TIMERDIAL")) ) {
		config_timerdial(argv[2]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
	}
    
    // AINPUT
	// ainput {0..%d} aname imin imax mmin mmax offset
	if (!strcmp_P( strupr(argv[1]), PSTR("AINPUT")) ) {
		ainputs_config_channel ( atoi(argv[2]), systemConf.ainputs_conf, argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
        pv_snprintfP_OK();
		return;
	}
    
    // COUNTER
    // counter {0..%d} cname magPP modo(PULSO/CAUDAL)
	if (!strcmp_P( strupr(argv[1]), PSTR("COUNTER")) ) {
        counters_config_channel( atoi(argv[2]), systemConf.counters_conf, argv[3], argv[4], argv[5] );
        pv_snprintfP_OK();
		return;
	}
    
    // DEBUG
    // config debug (ainput, counter, comms) (true,false)
    if (!strcmp_P( strupr(argv[1]), PSTR("DEBUG")) ) {
        config_debug( argv[2], argv[3]) ? pv_snprintfP_OK() : pv_snprintfP_ERR();
		return;
    }
    
    // CMD NOT FOUND
	xprintf("ERROR\r\nCMD NOT DEFINED\r\n\0");
	return;
 
}
//------------------------------------------------------------------------------
static void pv_snprintfP_OK(void )
{
	xprintf("ok\r\n\0");
}
//------------------------------------------------------------------------------
static void pv_snprintfP_ERR(void)
{
	xprintf("error\r\n\0");
}
//------------------------------------------------------------------------------