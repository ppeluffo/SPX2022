#include "spx2022.h"
#include "frtos_cmd.h"

/*
 * El RS485B esta conectado al UART0 que es el que atiende el modem
 * El buffer debe ser suficientemente grande para recibir los frames de
 * configuracion del servidor
 */

#define RS485B_BUFFER_SIZE 255

char rs485B_buffer[RS485B_BUFFER_SIZE];
lBuffer_s commsB_lbuffer;

void COMMSB_process_buffer( char c);

//------------------------------------------------------------------------------
void tkRS485B(void * pvParameters)
{

	// Esta es la primer tarea que arranca.

( void ) pvParameters;
uint8_t c = 0;
wan_port_t wan_port;

    while ( ! starting_flag )
        vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );

    lBchar_CreateStatic ( &commsB_lbuffer, rs485B_buffer, RS485B_BUFFER_SIZE );
    
    xprintf_P(PSTR("Starting tkRS485B..\r\n" ));
    
    wan_port = systemConf.wan_port;
    
	// loop
	for( ;; )
	{
        kick_wdt(XCMB_WDG_bp);
         
		c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.
		// el read se bloquea 50ms. lo que genera la espera.
        while ( xfgetc( fdRS485B, (char *)&c ) == 1 ) {
            // Vemos si este puerto esta configurado para la WAN
            if ( wan_port == WAN_RS485B ) {
                // Envio los datos a la cola WAN 
                WAN_put(c);
            } else {
                if ( lBchar_Put( &commsB_lbuffer, c) ) {
                    COMMSB_process_buffer(c);
                } else {
                    lBchar_Flush(&commsB_lbuffer);
                }
            }
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
