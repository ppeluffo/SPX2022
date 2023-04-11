#include "spx2022.h"
#include "frtos_cmd.h"
#include "modbus.h"

#define RS485A_BUFFER_SIZE 64

char rs485A_buffer[RS485A_BUFFER_SIZE];
lBuffer_s commsA_lbuffer;

//------------------------------------------------------------------------------
void tkRS485A(void * pvParameters)
{

	// Esta tarea maneja la cola de datos de la UART RS485A que es donde
    // se coloca el bus MODBUS.
    // Cada vez que hay un dato lo pone el un buffer circular commsA_lbuffer
    // para su procesamiento externo.

( void ) pvParameters;
uint8_t c = 0;

    while ( ! starting_flag )
        vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

	//vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );

    lBchar_CreateStatic ( &commsA_lbuffer, rs485A_buffer, RS485A_BUFFER_SIZE );
    
    /*
     * Este tarea recibe los datos del puerto A que es donde esta el bus modbus.
     * Debo inicializar el sistema modbus !!!
     */
    modbus_init( fdRS485A_MODBUS, RS485A_BUFFER_SIZE, MODBUS_flush_RXbuffer, MODBUS_getRXCount, MODBUS_RXBufferInit  );
    
    xprintf_P(PSTR("Starting tkRS485A..\r\n" ));
    
	// loop
	for( ;; )
	{
        kick_wdt(XCMA_WDG_bp);
         
		c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.
		// el read se bloquea 50ms. lo que genera la espera.
        while ( xfgetc( fdRS485A, (char *)&c ) == 1 ) {
            lBchar_Put( &commsA_lbuffer, c);
        }
        
        vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );
	}    
}
//------------------------------------------------------------------------------
void MODBUS_flush_RXbuffer(void)
{
    // Wrapper para usar en modbus_init
    
    lBchar_Flush( &commsA_lbuffer );
}
//------------------------------------------------------------------------------
uint16_t MODBUS_getRXCount(void)
{
    // Wrapper para usar en modbus_init
    
    return( lBchar_GetCount(&commsA_lbuffer) );
}
//------------------------------------------------------------------------------
char *MODBUS_RXBufferInit(void)
{
    // Wrapper para usar en modbus_init
    
    return ( lBchar_get_buffer(&commsA_lbuffer) );
}
//------------------------------------------------------------------------------
