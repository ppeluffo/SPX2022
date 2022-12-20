
/*
 * l_file.c
 *
 *  Created on: 31/10/2015
 *      Author: pablo
 */


#include "fileSystem.h"

SemaphoreHandle_t sem_FAT;
StaticSemaphore_t FAT_xMutexBuffer;
#define MSTOTAKEFATSEMPH ((  TickType_t ) 10 )

//#define DEBUG_FS
//------------------------------------------------------------------------------
bool FAT_flush(void)
{
    
int16_t xBytes;
bool retS = false;

    while ( xSemaphoreTake(sem_FAT, ( TickType_t ) 5 ) != pdTRUE )
		vTaskDelay( ( TickType_t)( 1 ) );
        
	FAT.head = 0;	// start
	FAT.tail = 0;	// end
	FAT.count = 0;
	FAT.length = FF_MAX_RCDS;
    
    xBytes = RTC_write( FAT_ADDRESS, (char *)&FAT, sizeof(fat_s) );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: I2C:RTC:FAT_flush\r\n"));
        goto quit;
    }
    retS = true;

quit:

    xSemaphoreGive( sem_FAT);
    return(retS);

}
//------------------------------------------------------------------------------
void FAT_read( fat_s *dstfat)
{
    memcpy( dstfat, &FAT, sizeof(fat_s));
}
//------------------------------------------------------------------------------
bool FS_open(void)
{
	// Inicializa el sistema del file system leyendo de la SRAM del RTC la FAT
	// y cargandola en memoria.
	// Debo chequear la consistencia y si esta mal indicarlo y reiniciarla

bool retS = false;
int16_t xBytes;

	while ( xSemaphoreTake(sem_FAT, ( TickType_t ) 5 ) != pdTRUE )
		vTaskDelay( ( TickType_t)( 1 ) );

    xBytes = RTC_read( FAT_ADDRESS, (char *)&FAT, sizeof(fat_s) );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: I2C:RTC:FAT_open\r\n"));
        goto quit;
    }
    
    FAT.length = FF_MAX_RCDS;
    xprintf_P( PSTR("FAT: head=%d, tail=%d,count=%d, length=%d\r\n"), FAT.head,FAT.tail, FAT.count, FAT.length );

    retS = true;
    
quit:
    xSemaphoreGive( sem_FAT);
	return(retS);

}
//------------------------------------------------------------------------------
void FS_init(void)
{
	sem_FAT = xSemaphoreCreateMutexStatic( &FAT_xMutexBuffer );
}
//------------------------------------------------------------------------------
int16_t FS_writeRcd( const void *dr, uint8_t xSize )
{
    /*
     * La eeprom actua como un ringbuffer.
     * Escribo en la posicion apuntada por head y lo avanzo en modo circular.
     * 
     */
    
uint16_t wrAddress;
int16_t bytes_written = -1;
int16_t xBytes;
int next;
int retV = -1;

    while ( xSemaphoreTake(sem_FAT, ( TickType_t ) 5 ) != pdTRUE )
		vTaskDelay( ( TickType_t)( 1 ) );

   // Buffer lleno

    if ( FAT.count == FAT.length) {
        xprintf_P(PSTR("ERROR: FS full.!! \r\n"));
        retV = -1;
        goto quit;
    }
    
    /*
     * Guardo
     * Primero pongo los datos en el buffer y agrego el checksum y end_tag.
     * Calculo la direccion interna de la eeprom.
     */

    memset( fs_rw_buffer, 0xFF, FF_RECD_SIZE );
	// copio los datos recibidos del dr al buffer ( 0..(xSize-1))
	memcpy ( fs_rw_buffer, dr, xSize );
	// Calculo y grabo el checksum a continuacion del frame (en la pos.xSize)
	// El checksum es solo del dataFrame por eso paso dicho size.
	fs_rw_buffer[FF_RECD_SIZE - 2] = fs_chksum8( fs_rw_buffer, (FF_RECD_SIZE - 2) );
	// Grabo el tag para indicar que el registro esta escrito.
	fs_rw_buffer[FF_RECD_SIZE - 1] = FF_WRTAG;
	// 
	wrAddress = FF_ADDR_START + FAT.head * FF_RECD_SIZE;
    vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );
	bytes_written = EE_write( wrAddress, (char *)&fs_rw_buffer, FF_RECD_SIZE );
    // Control de errores:
	if ( bytes_written != FF_RECD_SIZE ) {
		xprintf_P(PSTR("ERROR: I2C:EE:FS_writeRcd\r\n"));
        goto quit;
    }
    
#ifdef DEBUG_FS
    uint8_t i;
    
    for (i=0; i<FF_RECD_SIZE; i++) {
        if ( (i%8) == 0 ) {
            xprintf_P(PSTR("\r\n%02d: "),i);
        }
        xprintf_P(PSTR("[0x%02x] "), fs_rw_buffer[i]);
    }
    xprintf_P(PSTR("\r\n"));
#endif
    
    // Avanzo el puntero en forma circular
    FAT.count++;
    next = FAT.head + 1;
    if ( next >= FAT.length) {
        next = 0;
    }
    FAT.head = next;
    
    // Actualizo la fat por si se resetea el equipo
    xBytes = RTC_write( FAT_ADDRESS, (char *)&FAT, sizeof(fat_s) );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: I2C:RTC:FAT_save\r\n"));
        goto quit;
    }
    retV =bytes_written;
    
quit:

    xSemaphoreGive( sem_FAT);
    return(retV);
    
}
//------------------------------------------------------------------------------
int16_t FS_readRcd( void *dr, uint8_t xSize )
{
    /*
     * Leo de la eeprom de la posicion apuntada por tail.
     * Verifico que no este vacia.
     * 
     */

int next;
int16_t retV = -1;
uint8_t rdCheckSum = 0;
uint8_t calcCheckSum = 0;
uint16_t rdAddress = 0;
int16_t bytes_read = 0U;
int16_t xBytes;

	// Lo primero es obtener el semaforo
	while ( xSemaphoreTake(sem_FAT, ( TickType_t ) 5 ) != pdTRUE )
		vTaskDelay( ( TickType_t)( 1 ) );

    // Verifico que no este vacia.
    if ( FAT.count == 0 ) {   
        retV = -1;
        goto quit;
    }
    
    /*
     * Leo y avanzo el puntero.
     */
	memset( fs_rw_buffer, 0xFF, FF_RECD_SIZE );
	rdAddress = FF_ADDR_START + FAT.tail * FF_RECD_SIZE;
	bytes_read = EE_read( rdAddress, (char *)&fs_rw_buffer, FF_RECD_SIZE);
    // Copio los datos a la estructura de salida.: aun no se si estan correctos
	memcpy( dr, &fs_rw_buffer, xSize );
	if (bytes_read == -1 ) {
		xprintf_P(PSTR("ERROR: I2C:EE:FS_readRcd\r\n"));
        retV = -1;
        goto quit;
    }
    // Control de checksums
    calcCheckSum = fs_chksum8( fs_rw_buffer, (FF_RECD_SIZE - 2) );
    rdCheckSum = fs_rw_buffer[(FF_RECD_SIZE - 2)];
	if ( rdCheckSum != calcCheckSum ) {
        xprintf_P(PSTR("ERROR: I2C:EE:FS_readRcd checksum: calc=0x%02x, mem=0x%02x\r\n"),calcCheckSum, rdCheckSum );
        retV = -1;
		goto quit;
	}

	// Vemos si la ultima posicion tiene el tag de ocupado.
	if ( ( fs_rw_buffer[FF_RECD_SIZE - 1] )  != FF_WRTAG ) {
        xprintf_P(PSTR("ERROR: I2C:EE:FS_readRcd TAG\r\n"));
        retV = -1;
		goto quit;
	}
    
    // Está todo bien. Ajusto los punteros.
	FAT.count--;
    next =FAT.tail + 1;
    if ( next >= FAT.length) {
        next = 0;
    }
    FAT.tail = next;
    
    // Actualizo la fat por si se resetea el equipo
    xBytes = RTC_write( FAT_ADDRESS, (char *)&FAT, sizeof(fat_s) );
	if ( xBytes == -1 ) {
		xprintf_P(PSTR("ERROR: I2C:RTC:FAT_save\r\n"));
        goto quit;
    }
    retV = bytes_read;
    
quit:

    xSemaphoreGive( sem_FAT);
    return(retV);
}
//------------------------------------------------------------------------------
int16_t FS_readRcdByPos( uint16_t pos, void *dr, uint8_t xSize )
{
    /*
     * Leo de la eeprom de la posicion apuntada por tail.
     * Verifico que no este vacia.
     * 
     */

int16_t retV = -1;
uint8_t rdCheckSum = 0;
uint8_t calcCheckSum = 0;
uint16_t rdAddress = 0;
int16_t bytes_read = 0U;
uint8_t i;

	// Lo primero es obtener el semaforo
	while ( xSemaphoreTake(sem_FAT, ( TickType_t ) 5 ) != pdTRUE )
		vTaskDelay( ( TickType_t)( 1 ) );

    /*
     * Leo
     */
	memset( fs_rw_buffer, 0xFF, FF_RECD_SIZE );
	rdAddress = FF_ADDR_START + pos * FF_RECD_SIZE;
	bytes_read = EE_read( rdAddress, (char *)&fs_rw_buffer, FF_RECD_SIZE);
    // Copio los datos a la estructura de salida.: aun no se si estan correctos
	memcpy( dr, &fs_rw_buffer, xSize );
 
 //#ifdef DEBUG_FS
    for (i=0; i<FF_RECD_SIZE; i++) {
        if ( (i%8) == 0 ) {
            xprintf_P(PSTR("\r\n%02d: "),i);
        }
        xprintf_P(PSTR("[0x%02x] "), fs_rw_buffer[i]);
    }
    xprintf_P(PSTR("\r\n"));
//#endif
    
	if (bytes_read == -1 ) {
		xprintf_P(PSTR("ERROR: I2C:EE:FS_readRcd\r\n"));
        retV = -1;
        goto quit;
    }
    // Control de checksums
    calcCheckSum = fs_chksum8( fs_rw_buffer, (FF_RECD_SIZE - 2) );
    rdCheckSum = fs_rw_buffer[(FF_RECD_SIZE - 2)];
    xprintf_P(PSTR("FS_readRcdByPos:: checksum: calc=0x%02x, mem=0x%02x\r\n"),calcCheckSum, rdCheckSum );
	if ( rdCheckSum != calcCheckSum ) {
        retV = -1;
		goto quit;
	}

	// Vemos si la ultima posicion tiene el tag de ocupado.
	if ( ( fs_rw_buffer[FF_RECD_SIZE - 1] )  != FF_WRTAG ) {
        xprintf_P(PSTR("ERROR: I2C:EE:FS_readRcd TAG\r\n"));
        retV = -1;
		goto quit;
	}
    
    retV = bytes_read;
    
quit:

    xSemaphoreGive( sem_FAT);
    return(retV);
}
//------------------------------------------------------------------------------
void FS_format(bool fullformat)
{
	// Inicializa la memoria reseteando solo la FAT.
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Para que no salga por watchdog, apago las tareas previamente !!!!
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


uint16_t page = 0;
uint16_t wrAddress = 0;
int16_t xBytes = 0;

    FAT_flush();
    
	if ( fullformat ) {
        
        while ( xSemaphoreTake(sem_FAT, ( TickType_t ) 5 ) != pdTRUE )
            vTaskDelay( ( TickType_t)( 1 ) );

        memset( fs_rw_buffer, 0xFF, FF_RECD_SIZE );
	
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// Para que no salga por watchdog, apago las tareas previamente !!!!
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //xprintf_P( PSTR("FF_format: hard clear\r\n"));
		// Borro fisicamente los registros
		for ( page = 0; page < FF_MAX_RCDS; page++ ) {
			wrAddress = FF_ADDR_START + page * FF_RECD_SIZE;
			xBytes = EE_write( wrAddress, (char *)&fs_rw_buffer, FF_RECD_SIZE );
			if ( xBytes == -1 )
				xprintf_P(PSTR("ERROR: I2C:EE:FS_format\r\n"));

			vTaskDelay( ( TickType_t)( 10 ) );

			if ( ( page > 0 ) && (page % 32) == 0 ) {
				xprintf_P( PSTR(" %04d"),page);
				if ( ( page > 0 ) && (page % 256) == 0 ) {
					xprintf_P( PSTR("\r\n"));
				}
			}
		}

        xSemaphoreGive( sem_FAT);
	}
   
}
//------------------------------------------------------------------------------
uint8_t fs_chksum8(const char *buff, size_t len)
{
uint8_t checksum = 0;

	for ( checksum = 0 ; len != 0 ; len-- )
		checksum += *(buff++);   // parenthesis not required!

	checksum = ~checksum;
	return (checksum);
}
//------------------------------------------------------------------------------
uint16_t FS_dump( bool (*funct)(char *buff) )
{
    /*
     * Leo todos los registros y los proceso de a uno con la funcion
     * pasada como parámetro.
     * Esta puede imprimirlos en pantalla o copiarlos y enviarlos.
     */
    
uint16_t ptr;
uint16_t rdAddress = 0;
uint16_t i;
int16_t bytes_read = 0U;
bool fRet;

	while ( xSemaphoreTake(sem_FAT, ( TickType_t ) 5 ) != pdTRUE )
		vTaskDelay( ( TickType_t)( 1 ) );

    ptr = FAT.tail;
    
    for (i=0; i < FAT.count; i++) {
        //xprintf_P(PSTR("PTR=%d, I=%d\r\n"),ptr,i);
        memset( fs_rw_buffer, 0x00, FF_RECD_SIZE );
        rdAddress = FF_ADDR_START + ptr * FF_RECD_SIZE;
        bytes_read = EE_read( rdAddress, (char *)&fs_rw_buffer, FF_RECD_SIZE);  
        // Avanzo el puntero(circular)
        ptr++;
        if ( ptr >= FAT.length) {
            ptr = 0;
        }
        // Ejecuto la funcion que procesa el buffer.
        fRet = funct(&fs_rw_buffer[0]);
        
        // Si algun registro me da error, salgo.
        if ( !fRet)
            break;
        
    }
    
    xprintf_P( PSTR("MEM bd EMPTY\r\n"));
    
    xSemaphoreGive( sem_FAT);
    
    // Retorno la cantidad de registros procesados.
    return(i);

}
//------------------------------------------------------------------------------
void FS_delete( int16_t ndrcds )
{
    /*
     * Borra del FS ndrcds registros.
     * Comienza en el head y va avanzando.
     * Si ndrcds = -1, borra toda la memoria.
     */

uint16_t i;
uint16_t max_rcds;
uint16_t wrAddress = 0;
int16_t xBytes = 0;
int16_t bytes_written = -1;
int next;

    while ( xSemaphoreTake(sem_FAT, ( TickType_t ) 5 ) != pdTRUE )
		vTaskDelay( ( TickType_t)( 1 ) );

    // Calculo los limites
    if (ndrcds == -1) {
        max_rcds = FAT.count;
    } else {
        max_rcds = ndrcds;
    }

     /*
     * Borro y avanzo el puntero.
     */
	memset( fs_rw_buffer, 0xFF, FF_RECD_SIZE );
    for (i=0; i<max_rcds; i++) {
        
        vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );
        
        // Verifico que no este vacia.
        if ( FAT.count == 0 ) {   
            goto quit;
        }
 
        // Borro
        wrAddress = FF_ADDR_START + FAT.tail * FF_RECD_SIZE;      
        bytes_written = EE_write( wrAddress, (char *)&fs_rw_buffer, FF_RECD_SIZE );
        // Control de errores:
        if ( bytes_written != FF_RECD_SIZE ) {
            xprintf_P(PSTR("ERROR: I2C:EE:FS_delete(%d)\r\n"),FAT.tail);
            //goto quit;
        }
    
        // Está todo bien. Ajusto los punteros.
        FAT.count--;
        next =FAT.tail + 1;
        if ( next >= FAT.length) {
            next = 0;
        }
        FAT.tail = next;
    
        // Actualizo la fat por si se resetea el equipo
        xBytes = RTC_write( FAT_ADDRESS, (char *)&FAT, sizeof(fat_s) );
        if ( xBytes == -1 ) {
            xprintf_P(PSTR("ERROR: I2C:RTC:FAT_save\r\n"));
            goto quit;
        }
    }
    
quit:

    xSemaphoreGive( sem_FAT);
}
//------------------------------------------------------------------------------