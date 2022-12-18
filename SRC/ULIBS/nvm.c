/*
 * l_nvm.c
 *
 *  Created on: 18 feb. 2019
 *      Author: pablo
 */

#include "nvm.h"

char nvm_str_buffer[38];

//------------------------------------------------------------------------------
uint16_t nvm_read_signature_sum(void)
{
nvm_device_serial_id_t avr_id;
uint16_t signature_sum=0;  

    NVM_ID_read( &avr_id );
    signature_sum += avr_id.devid0;
    signature_sum += avr_id.devid1;
    signature_sum += avr_id.devid2;
    signature_sum += avr_id.sernum0;
    signature_sum += avr_id.sernum1;
    signature_sum += avr_id.sernum2;
    signature_sum += avr_id.sernum3;
    signature_sum += avr_id.sernum4;
    signature_sum += avr_id.sernum5;
    signature_sum += avr_id.sernum6;
    signature_sum += avr_id.sernum7;
    signature_sum += avr_id.sernum8;
    signature_sum += avr_id.sernum9;
    signature_sum += avr_id.sernum10;
    signature_sum += avr_id.sernum11;
    signature_sum += avr_id.sernum12;
    signature_sum += avr_id.sernum13;
    signature_sum += avr_id.sernum14;
    signature_sum += avr_id.sernum15;
    
    return(signature_sum);
}
//------------------------------------------------------------------------------
char *NVM_id2str(void)
{
    /*
     * Retorna el ID en el string dst
     */

nvm_device_serial_id_t avr_id;
uint8_t fptr = 0;

    memset(nvm_str_buffer, '\0', sizeof(nvm_str_buffer));
    NVM_ID_read( &avr_id );
    fptr += sprintf_P( (char *)&nvm_str_buffer[fptr], PSTR("0x%02x%02x%02x"), avr_id.devid0, avr_id.devid1, avr_id.devid2);  
    return( &nvm_str_buffer[0] );
}
//------------------------------------------------------------------------------
char *NVM_signature2str( void )
{
    /*
     * Retorna el SIGNATURE en el string dst
     */

nvm_device_serial_id_t avr_id;
uint8_t fptr = 0;

    memset(nvm_str_buffer, '\0', sizeof(nvm_str_buffer));
    NVM_ID_read( &avr_id );  
    
    fptr = 0;
    fptr += sprintf_P( (char *)&nvm_str_buffer[fptr], PSTR("%02x%02x%02x%02x"), avr_id.sernum0, avr_id.sernum1, avr_id.sernum2, avr_id.sernum3);
    fptr += sprintf_P( (char *)&nvm_str_buffer[fptr], PSTR("%02x%02x%02x%02x"), avr_id.sernum4, avr_id.sernum5, avr_id.sernum6, avr_id.sernum7);
    fptr += sprintf_P( (char *)&nvm_str_buffer[fptr] ,PSTR("%02x%02x%02x%02x"), avr_id.sernum8, avr_id.sernum9, avr_id.sernum10, avr_id.sernum11);
    fptr += sprintf_P( (char *)&nvm_str_buffer[fptr], PSTR("%02x%02x%02x%02x"), avr_id.sernum12, avr_id.sernum13, avr_id.sernum14, avr_id.sernum15);
    
    return( &nvm_str_buffer[0] );
}
//------------------------------------------------------------------------------
void nvm_read_print_id(void)
{
nvm_device_serial_id_t avr_id;

    NVM_ID_read( &avr_id );
    //xprintf_P(PSTR("AVR_ID: 0x%02x 0x%02x 0x%02x\r\n"), avr_id.devid0, avr_id.devid1, avr_id.devid2);
    //xprintf_P(PSTR("SIGNATURE: 0x%02x 0x%02x 0x%02x 0x%02x "), avr_id.sernum0, avr_id.sernum1, avr_id.sernum2, avr_id.sernum3);
    //xprintf_P(PSTR("0x%02x 0x%02x 0x%02x 0x%02x "), avr_id.sernum4, avr_id.sernum5, avr_id.sernum6, avr_id.sernum7);
    //xprintf_P(PSTR("0x%02x 0x%02x 0x%02x 0x%02x "), avr_id.sernum8, avr_id.sernum9, avr_id.sernum10, avr_id.sernum11);
    //xprintf_P(PSTR("0x%02x 0x%02x 0x%02x 0x%02x\r\n"), avr_id.sernum12, avr_id.sernum13, avr_id.sernum14, avr_id.sernum15);

    xprintf_P(PSTR("AVR_ID: 0x%02x%02x%02x\r\n"), avr_id.devid0, avr_id.devid1, avr_id.devid2);
    xprintf_P(PSTR("SIGNATURE: %02x%02x%02x%02x"), avr_id.sernum0, avr_id.sernum1, avr_id.sernum2, avr_id.sernum3);
    xprintf_P(PSTR("%02x%02x%02x%02x"), avr_id.sernum4, avr_id.sernum5, avr_id.sernum6, avr_id.sernum7);
    xprintf_P(PSTR("%02x%02x%02x%02x"), avr_id.sernum8, avr_id.sernum9, avr_id.sernum10, avr_id.sernum11);
    xprintf_P(PSTR("%02x%02x%02x%02x\r\n"), avr_id.sernum12, avr_id.sernum13, avr_id.sernum14, avr_id.sernum15);
    
}
//------------------------------------------------------------------------------
void NVM_ID_read( nvm_device_serial_id_t *avr_id )
{
	// El signature lo leo una sola vez.
	// Luego, como lo tengo en la memoria, no lo leo mas.

    avr_id->devid0 = SIGROW.DEVICEID0;
    avr_id->devid1 = SIGROW.DEVICEID1;
    avr_id->devid2 = SIGROW.DEVICEID2;
    
    avr_id->sernum0 = SIGROW.SERNUM0;
    avr_id->sernum1 = SIGROW.SERNUM1;
    avr_id->sernum2 = SIGROW.SERNUM2;
    avr_id->sernum3 = SIGROW.SERNUM3;
    avr_id->sernum4 = SIGROW.SERNUM4;
    avr_id->sernum5 = SIGROW.SERNUM5;
    avr_id->sernum6 = SIGROW.SERNUM6;
    avr_id->sernum7 = SIGROW.SERNUM7;
    avr_id->sernum8 = SIGROW.SERNUM8;
    avr_id->sernum9 = SIGROW.SERNUM9;
    avr_id->sernum10 = SIGROW.SERNUM10;
    avr_id->sernum11 = SIGROW.SERNUM11;
    avr_id->sernum12 = SIGROW.SERNUM12;
    avr_id->sernum13 = SIGROW.SERNUM13; 
    avr_id->sernum14 = SIGROW.SERNUM14;
    avr_id->sernum15 = SIGROW.SERNUM15;
                 
}
//------------------------------------------------------------------------------------
int8_t NVMEE_read( uint16_t eeRdAddr, char *data, size_t size )
{
    frtos_ioctl( fdNVM, ioctl_OBTAIN_BUS_SEMPH, NULL);
    frtos_ioctl( fdNVM, ioctl_NVM_SET_EEADDRESS, &eeRdAddr);
   
    frtos_read( fdNVM, data , size );
    
    frtos_ioctl( fdNVM,ioctl_RELEASE_BUS_SEMPH, NULL);
    
    return(size);
    
}
//------------------------------------------------------------------------------
int8_t NVMEE_write( uint16_t eeWrAddr, char *data, size_t size )
{

int16_t retValue = -1;
    
    frtos_ioctl( fdNVM, ioctl_OBTAIN_BUS_SEMPH, NULL);
    frtos_ioctl( fdNVM, ioctl_NVM_SET_EEADDRESS, &eeWrAddr);
    
    retValue = frtos_write( fdNVM, data , size );
    
    frtos_ioctl( fdNVM,ioctl_RELEASE_BUS_SEMPH, NULL);
    
    return(retValue);
	
}
//------------------------------------------------------------------------------
void NVMEE_test_read( char *addr, char *size )
{
	// Funcion de testing de la EEPROM interna del micro xmega
	// Lee de una direccion de la memoria una cantiad de bytes y los imprime
	// parametros: *addr > puntero char a la posicion de inicio de lectura
	//             *size >  puntero char al largo de bytes a leer
	// retorna: -1 error
	//			nro.de bytes escritos

char buffer[32] = { 0 };
int length = (uint8_t)(atoi( size));

	NVMEE_read( (uint16_t)(atoi(addr)), buffer, length );
	buffer[length] = '\0';
	xprintf_P( PSTR( "%s\r\n"),buffer);

}
//------------------------------------------------------------------------------------
void NVMEE_test_write( char *addr, char *str )
{
	// Funcion de testing de la EEPROM interna del micro xmega
	// Escribe en una direccion de memoria un string
	// parametros: *addr > puntero char a la posicion de inicio de escritura
	//             *str >  puntero char al texto a escribir
	// retorna: -1 error
	//			nro.de bytes escritos

	// Calculamos el largo del texto a escribir en la eeprom.

uint8_t length = 0;
char *p = NULL;


	p = str;
	while (*p != 0) {
		p++;
		length++;
	}

	NVMEE_write( (uint16_t)(atoi(addr)), str, length );
	xprintf_P( PSTR( "wr %d bytes\r\n"),length);
	return;


}
//------------------------------------------------------------------------------------