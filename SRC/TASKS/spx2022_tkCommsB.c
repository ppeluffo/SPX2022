#include "spx2022.h"
#include "frtos_cmd.h"


#define RS485B_BUFFER_SIZE 64

char rs485B_buffer[RS485B_BUFFER_SIZE];
lBuffer_s commsB_lbuffer;

void COMMSB_process_buffer( char c);

//------------------------------------------------------------------------------
void tkCommsB(void * pvParameters)
{

	// Esta es la primer tarea que arranca.

( void ) pvParameters;
uint8_t c = 0;

    while ( ! starting_flag )
        vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

	//vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );

    lBchar_CreateStatic ( &commsB_lbuffer, rs485B_buffer, RS485B_BUFFER_SIZE );
    
    xprintf_P(PSTR("Starting tkCommsB..\r\n" ));
    
	// loop
	for( ;; )
	{
        kick_wdt(XCMB_WDG_bp);
         
		c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.
		// el read se bloquea 50ms. lo que genera la espera.
        while ( xfgetc( fdRS485B, (char *)&c ) == 1 ) {
            lBchar_Put( &commsB_lbuffer, c);
            COMMSB_process_buffer(c);
        }
        
        vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );
	}    
}
//------------------------------------------------------------------------------
void COMMSB_process_buffer( char c)
{
 
    if (( c == '\n') || ( c == '\r')) {
        
        xprintf_P(PSTR("COMMSB RCVD:>%s\r\n"), lBchar_get_buffer(&commsB_lbuffer));
        lBchar_Flush(&commsB_lbuffer);
    }
}
//------------------------------------------------------------------------------
