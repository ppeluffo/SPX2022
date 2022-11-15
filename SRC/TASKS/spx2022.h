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
#define FW_DATE "@ 20221115"
#define HW_MODELO "SPX2022 FRTOS R001 HW:AVR128DA64"
#define FRTOS_VERSION "FW:FreeRTOS V202111.00"
#define FW_TYPE "SPX"

#define SYSMAINCLK 24

#define tkCtl_TASK_PRIORITY	 	( tskIDLE_PRIORITY + 1 )
#define tkCmd_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkAin_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkCnt_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkSys_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkCommsA_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )
#define tkCommsB_TASK_PRIORITY 	( tskIDLE_PRIORITY + 1 )

#define tkCtl_STACK_SIZE		384
#define tkCmd_STACK_SIZE		384
#define tkAin_STACK_SIZE		384
#define tkCnt_STACK_SIZE		384
#define tkSys_STACK_SIZE		384
#define tkCommsA_STACK_SIZE		384
#define tkCommsB_STACK_SIZE		384

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

StaticTask_t tkCommsA_Buffer_Ptr;
StackType_t tkCommsA_Buffer [tkCommsA_STACK_SIZE];

StaticTask_t tkCommsB_Buffer_Ptr;
StackType_t tkCommsB_Buffer [tkCommsB_STACK_SIZE];

SemaphoreHandle_t sem_SYSVars;
StaticSemaphore_t SYSVARS_xMutexBuffer;
#define MSTOTAKESYSVARSSEMPH ((  TickType_t ) 10 )

TaskHandle_t xHandle_tkCtl, xHandle_tkCmd, xHandle_tkAin, xHandle_tkCnt, xHandle_tkSys, xHandle_tkCommsA, xHandle_tkCommsB;

void tkCtl(void * pvParameters);
void tkCmd(void * pvParameters);
void tkSystem(void * pvParameters);
void tkAinputs(void * pvParameters);
void tkCounters(void * pvParameters);
void tkCommsA(void * pvParameters);
void tkCommsB(void * pvParameters);

void system_init();
void reset(void);

void kick_wdt( uint8_t bit_pos);

void config_default(void);
bool config_debug( char *tipo, char *valor);
bool save_config_in_NVM(void);
bool load_config_from_NVM(void);

bool starting_flag;

#define DLGID_LENGTH		12

struct {   
    bool debug;
    float ainputs[NRO_ANALOG_CHANNELS];
    float counters[NRO_COUNTER_CHANNELS];
} systemVars;

struct {
    char dlgid[DLGID_LENGTH];
    uint16_t timerpoll;
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

#define WDG_bm 0x2F

#define WDG_INIT() ( sys_watchdog = WDG_bm )


#endif	/* XC_HEADER_TEMPLATE_H */

