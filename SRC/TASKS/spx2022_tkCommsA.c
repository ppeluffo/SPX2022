#include "spx2022.h"
#include "frtos_cmd.h"


//------------------------------------------------------------------------------
void tkRS485A(void * pvParameters)
{

	// Esta es la primer tarea que arranca.

( void ) pvParameters;

    while ( ! starting_flag )
        vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

	//vTaskDelay( ( TickType_t)( 500 / portTICK_PERIOD_MS ) );

uint8_t c = 0;
    
    xprintf_P(PSTR("Starting tkRS485A..\r\n" ));
    
	// loop
	for( ;; )
	{
        kick_wdt(XCMA_WDG_bp);
         
		c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.
		// el read se bloquea 50ms. lo que genera la espera.
		//while ( frtos_read( fdTERM, (char *)&c, 1 ) == 1 ) {
        while ( xfgetc( fdRS485A, (char *)&c ) == 1 ) {
            vTaskDelay( ( TickType_t)( 1 / portTICK_PERIOD_MS ) );
        }
        
        vTaskDelay( ( TickType_t)( 10 / portTICK_PERIOD_MS ) );
	}    
}
//------------------------------------------------------------------------------

