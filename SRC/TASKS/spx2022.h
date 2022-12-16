/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H


#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    // TODO If C++ is being used, regular C code needs function names to have C 
    // linkage so the functions can be used by the c code. 

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#ifndef F_CPU
#define F_CPU 24000000
#endif


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "list.h"
#include "croutine.h"
#include "semphr.h"
#include "timers.h"
#include "limits.h"
#include "portable.h"

#include "protected_io.h"
#include "ccp.h"

#include <avr/io.h>
#include <avr/builtins.h>
#include <avr/wdt.h> 
#include <avr/pgmspace.h>
#include <avr/fuse.h>
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#include "frtos-io.h"
#include "xprintf.h"
#include "xgetc.h"
#include "i2c.h"
#include "eeprom.h"
#include "rtc79410.h"
#include "nvm.h"
#include "led.h"
#include "counters.h"
#include "pines.h"
#include "linearBuffer.h"
#include "ainputs.h"
#include "counters.h"

#define FW_REV "1.0.0"
#define FW_DATE "@ 20221214"
#define HW_MODELO "SPX2022 FRTOS R001 HW:AVR128DA64"
#define FRTOS_VERSION "FW:FreeRTOS V202111.00"
#define FW_TYPE "SPXR2"

#define SYSMAINCLK 24

#define tkCtl_TASK_PRIORITY	 	( tskIDLE_PRIORITY + 1 )
#define tkCmd_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkAin_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkCnt_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkSys_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkRS485A_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkRS485B_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkWAN_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )

#define tkCtl_STACK_SIZE		384
#define tkCmd_STACK_SIZE		384
#define tkAin_STACK_SIZE		384
#define tkCnt_STACK_SIZE		384
#define tkSys_STACK_SIZE		384
#define tkRS485A_STACK_SIZE		384
#define tkRS485B_STACK_SIZE		384
#define tkWAN_STACK_SIZE		384

StaticTask_t tkCtl_Buffer_Ptr;
StackType_t tkCtl_Buffer [tkCtl_STACK_SIZE];

StaticTask_t tkCmd_Buffer_Ptr;
StackType_t tkCmd_Buffer [tkCmd_STACK_SIZE];

StaticTask_t tkAin_Buffer_Ptr;
StackType_t tkAin_Buffer [tkAin_STACK_SIZE];

StaticTask_t tkCnt_Buffer_Ptr;
StackType_t tkCnt_Buffer [tkCnt_STACK_SIZE];

StaticTask_t tkSys_Buffer_Ptr;
StackType_t tkSys_Buffer [tkSys_STACK_SIZE];

StaticTask_t tkRS485A_Buffer_Ptr;
StackType_t tkRS485A_Buffer [tkRS485A_STACK_SIZE];

StaticTask_t tkRS485B_Buffer_Ptr;
StackType_t tkRS485B_Buffer [tkRS485B_STACK_SIZE];

StaticTask_t tkWAN_Buffer_Ptr;
StackType_t tkWAN_Buffer [tkWAN_STACK_SIZE];

SemaphoreHandle_t sem_SYSVars;
StaticSemaphore_t SYSVARS_xMutexBuffer;
#define MSTOTAKESYSVARSSEMPH ((  TickType_t ) 10 )

TaskHandle_t xHandle_tkCtl, xHandle_tkCmd, xHandle_tkAin, xHandle_tkCnt, xHandle_tkSys, xHandle_tkRS485A, xHandle_tkRS485B, xHandle_tkWAN;

void tkCtl(void * pvParameters);
void tkCmd(void * pvParameters);
void tkSystem(void * pvParameters);
void tkAinputs(void * pvParameters);
void tkCounters(void * pvParameters);
void tkRS485A(void * pvParameters);
void tkRS485B(void * pvParameters);
void tkWAN(void * pvParameters);

void system_init();
void reset(void);

#define XPRINT_HEADER       true
#define XPRINT_NO_HEADER    false

#define TDIAL_MIN_DISCRETO  900

typedef struct {
    float l_ainputs[NRO_ANALOG_CHANNELS];
    float l_counters[NRO_COUNTER_CHANNELS];
} dataRcd_s;

void kick_wdt( uint8_t bit_pos);

uint8_t u_hash(uint8_t seed, char ch );
void config_default(void);
bool config_debug( char *tipo, char *valor);
bool save_config_in_NVM(void);
bool load_config_from_NVM(void);
bool config_wan_port(char *comms_type);
void xprint_terminal(bool print_header);
void data_resync_clock( char *str_time, bool force_adjust);
bool config_timerdial ( char *s_timerdial );
bool config_timerpoll ( char *s_timerpoll );
bool config_pwrmodo ( char *s_pwrmodo );
bool config_pwron ( char *s_pwron );
bool config_pwroff ( char *s_pwroff );
void print_pwr_configuration(void);


#define WAN_RX_BUFFER_SIZE 300
char wan_buffer[WAN_RX_BUFFER_SIZE];
lBuffer_s wan_lbuffer;

bool WAN_xmit_data_frame( dataRcd_s *dataRcd);
void WAN_print_configuration(void);
void WAN_config_debug(bool debug );
bool WAN_read_debug(void);
void WAN_put(uint8_t c);

bool starting_flag;

#define DLGID_LENGTH		12

struct {   
    bool debug;
    bool rele_output;
    float ainputs[NRO_ANALOG_CHANNELS];
    float counters[NRO_COUNTER_CHANNELS];
} systemVars;

typedef enum { WAN_RS485B = 0, WAN_NBIOT } wan_port_t;

typedef enum { PWR_CONTINUO = 0, PWR_DISCRETO, PWR_MIXTO } pwr_modo_t;

struct {
    wan_port_t wan_port;
    char dlgid[DLGID_LENGTH];
    uint16_t timerpoll;
    uint16_t timerdial;
    pwr_modo_t pwr_modo;
    uint16_t pwr_hhmm_on;
    uint16_t pwr_hhmm_off;
	ainputs_conf_t ainputs_conf[NRO_ANALOG_CHANNELS];
    counter_conf_t counters_conf[NRO_COUNTER_CHANNELS];
    uint8_t checksum;
} systemConf;


uint8_t sys_watchdog;

#define CMD_WDG_bp    0
#define SYS_WDG_bp    1
#define AIN_WDG_bp    2
#define CNT_WDG_bp    3
#define XCMA_WDG_bp   4
#define XCMB_WDG_bp   5
#define XWAN_WDG_bp   6

#define WDG_bm 0x7F

#define WDG_INIT() ( sys_watchdog = WDG_bm )


#endif	/* XC_HEADER_TEMPLATE_H */

