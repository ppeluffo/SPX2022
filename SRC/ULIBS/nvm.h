/*
 * l_nvm.h
 *
 *  Created on: 18 feb. 2019
 *      Author: pablo
 */

#ifndef SRC_SPX_LIBS_L_NVM_L_NVM_H_
#define SRC_SPX_LIBS_L_NVM_L_NVM_H_

#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "xprintf.h"
#include "stdbool.h"
#include "string.h"
#include "frtos-io.h"

typedef struct {
	union {
		struct {
			uint8_t devid0;
			uint8_t devid1;
			uint8_t devid2;
			uint8_t sernum0;
            uint8_t sernum1;
            uint8_t sernum2;
            uint8_t sernum3;
            uint8_t sernum4;
            uint8_t sernum5;
            uint8_t sernum6;
            uint8_t sernum7;
            uint8_t sernum8;
            uint8_t sernum9;
            uint8_t sernum10;
            uint8_t sernum11;
            uint8_t sernum12;
            uint8_t sernum13;
            uint8_t sernum14;
            uint8_t sernum15;

		};
		uint8_t byte[19];
	};
} nvm_device_serial_id_t;

void NVM_ID_read( nvm_device_serial_id_t *avr_id );
int8_t NVMEE_write( uint16_t eeWrAddr, char *data, size_t size );
int8_t NVMEE_read( uint16_t eeRdAddr, char *data, size_t size );
uint16_t nvm_read_signature_sum(void);

void nvm_read_print_id(void);

void NVMEE_test_write( char *addr, char *str );
void NVMEE_test_read( char *addr, char *size );

char *NVM_id2str(void);
char *NVM_signature2str( void );


#endif /* SRC_SPX_LIBS_L_NVM_L_NVM_H_ */
