/*
 * Los factores a considerar para mejorar el alcance son el 'spread factor',
 * 'bandwidth'.
 * 
 * Lora genera una señalizacion de datos binarios en el cual cada bit ocupa un
 * tiempo dado de la señal en el aire. Este tiempo es proporcional a 2^spread_factor.
 * De este modo, cuanto mayot spread_factor, mayor tiempo ocupa un bit pero esto 
 * lo hace mas robusto frente al ruido lo que hace que sea mas facil detectarlo.
 * Como contra, al ocupar mas tiempo, podemos transmitir menos bits.
 * Cada aumento de una unidad del spread_factor mejora el enlace en 2.5dB
 * A su vez, duplica el tiempo de transmitir 1 bit.
 * 
 * Ancho de banda:
 * La trasmision de c/bit ocupara todo el ancho de banda. Un BW mas alto tiene
 * velocidades mayores pero tiene mas congestion y por lo tanto menor alcance.
 * Cada duplicacion del BW reduce en 3dB el enlace.
 * 
 * Las desviaciones de la frecuencia no pueden superar el 25% del ancho de banda
 * de modulacion.
 * 
 * Para conseguir el rango mas largo debemos:
 * - Ancho de banda mas bajo posible
 * - Factor de dispersion mas alto posible
 * - Mayor potencia posible
 * 
 */
#include "lora.h"
#include "terminal_lora_pmp.h"


// -----------------------------------------------------------------------------
void LORA_init(void)
{
    LORA_RTS_CONFIG();
    LORA_CTS_CONFIG();
    LORA_RESET_CONFIG();
    
    // Mantengo el modulo 'en reset' para que no mande mensajes todavia.
    lora_reset_off();
    
    lBchar_CreateStatic(&lora_rx_sdata, lora_rx_buffer, LORA_RX_BUFFER_SIZE );
    lBchar_CreateStatic(&lora_tx_sdata, lora_tx_buffer, LORA_TX_BUFFER_SIZE );
    lBchar_CreateStatic(&lora_decoded_rx_sdata, lora_decoded_rx_buffer, LORA_DECODED_RX_BUFFER_SIZE );
}
// -----------------------------------------------------------------------------
void lora_flash_led(void)
{
    xprintf( "Lora test led.\r\n");
    
    lBchar_Flush(&lora_tx_sdata);
    sprintf( lBchar_get_buffer(&lora_tx_sdata), "sys set pindig GPIO5 1" );
    xfprintf( fdTERM, "[%s]\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    xfprintf( fdLORA, "%s\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    
    vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
    
    lBchar_Flush(&lora_tx_sdata);
    sprintf( lBchar_get_buffer(&lora_tx_sdata), "sys set pindig GPIO5 0" );
    xfprintf( fdTERM, "[%s]\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    xfprintf( fdLORA, "%s\r\n", lBchar_get_buffer(&lora_tx_sdata) );

}
// -----------------------------------------------------------------------------
void lora_responses_clear(void)
{
    radio_responses.rsp_ok = false;
    radio_responses.rsp_invalid_params = false;
    radio_responses.rsp_busy = false;
    radio_responses.rsp_radio_rx = false;
    radio_responses.rsp_radio_tx_ok = false;
    radio_responses.rsp_radio_err = false;
    
}
//------------------------------------------------------------------------------
void lora_responses_print(void)
{
   xprintf_P(PSTR("ok:%d\r\n"),radio_responses.rsp_ok);
   xprintf_P(PSTR("invalid_params:%d\r\n"),radio_responses.rsp_invalid_params);
   xprintf_P(PSTR("busy:%d\r\n"),radio_responses.rsp_busy);
   xprintf_P(PSTR("radio_rx:%d\r\n"),radio_responses.rsp_radio_rx);
   xprintf_P(PSTR("radio_tx_ok:%d\r\n"),radio_responses.rsp_radio_tx_ok);
   xprintf_P(PSTR("radio_err:%d\r\n"),radio_responses.rsp_radio_err);
}
//------------------------------------------------------------------------------
void lora_send_cmd(char **argv )
{
    
uint8_t i;
uint8_t j;
char *p;

    //xprintf_P(PSTR("LORA CMD\r\n"));
    
    lBchar_Flush(&lora_tx_sdata);
    lBchar_Flush(&lora_rx_sdata);
    
    // Genero el comando a mandar al modulo lora.
    // Arranco de 1 porque no quiero el 'lora '
    // El argumento 1 lo paso a minuscula porque el parseo lo cambio
    if ( argv[1] != NULL ) {
         argv[1] = strlwr(argv[1]);
    }
    
    p = lBchar_get_buffer(&lora_tx_sdata);
    j=0;
    for (i=1; i<16; i++) {
        if ( argv[i] != NULL ) {
            //xprintf_P(PSTR("arg#%02d: %s\r\n"), i, argv[i]);
            j += snprintf( &p[j], ( LORA_TX_BUFFER_SIZE - j - 1), "%s ", argv[i] );
        }
    }
    
    // Al final quedo un espacion en blanco que debo eliminarlo  
    p[ strlen(p) - 1] = '\0'; 
    
    xfprintf( fdTERM, "[%s]\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
    xfprintf( fdLORA, "%s\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
    
}
//------------------------------------------------------------------------------
bool lora_send_msg_in_ascii(char *msg)
{
    /*
     * Convierte en mensaje en ASCII sobre el lora_tx_buffer y luego 
     * manda el comando.
     * La respuesta al comando puede ser: { ok, invalid_param, busy }
     * La respuesta despues de transmitirlo puede ser: {radio_tx_ok, radio_err }
     * 
     * En caso que la respuesta no sea 'radio_tx_ok', reintentamos 3 veces.
     * 
     * Al finalizar volvemos al modo recepcion.
     */
  
uint8_t i = 0;
char *p1;
char *p2;
char c;
uint8_t timeout;
bool retS = false;

    lBchar_Flush(&lora_tx_sdata);
    p1 = lBchar_get_buffer(&lora_tx_sdata); 
    // Armo el frame.
    i += snprintf( &p1[i], LORA_TX_BUFFER_SIZE, "radio tx ");   
    
    p2=msg;
    while (*p2) {
        c=*p2++;
        i += snprintf( &p1[i], LORA_TX_BUFFER_SIZE, "%X",c); 
        //xprintf_P(PSTR("%c=%x\r\n"), c, c);
    }
    
    // Espero el semaforo para capturar el radio
	while ( xSemaphoreTake( sem_LORA, ( TickType_t ) 5 ) != pdTRUE )
		vTaskDelay( ( TickType_t)( 10 ) );
    
    //lora_flush_RxBuffer();
    lBchar_Flush(&lora_rx_sdata);
    
    lora_responses_clear();
    xfprintf( fdTERM, "TX=[%s]\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    xfprintf( fdLORA, "%s\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    
    // Espero alguna respuesta hasta 5 secs.
    timeout = 0;
    while( timeout++ < 50) {
        // OK. Envio el mensaje correctamente
        if ( radio_responses.rsp_radio_tx_ok ) {
            retS = true;
            break;
        }    
        vTaskDelay( ( TickType_t)( 50 / portTICK_PERIOD_MS ) );
    }
    
    xSemaphoreGive( sem_LORA );
    return (retS);
}
//------------------------------------------------------------------------------
void lora_print_rxvd_msg(void)
{
    /*
     Decodifica el mensaje que hay en el rxBuffer.
     */
    
char *p = NULL;

    // Elimino todo hasta el 'radio_rx '
    p = lBchar_get_buffer(&lora_rx_sdata); 

    if (p == NULL)
        return;

    // Elimino posibles caracteres al inicio
    while (*p != ' ')
        p++;

    // Elimino los caracteres en blanco que quedan
    while (*p == ' ')
        p++;
    
    xprintf_P(PSTR("RXMSG:[%s]\r\n"),p);
    
}
//------------------------------------------------------------------------------
void lora_decode_msg(void)
{
    /*
     Decodifica el mensaje que hay en el rxBuffer.
     */
    
char *p = NULL;
uint8_t c0,c1;
uint8_t d0,d1;
uint8_t dec;


    lBchar_Flush(&lora_decoded_rx_sdata);
    
    // Elimino todo hasta el 'radio_rx '
    p = lBchar_get_buffer(&lora_rx_sdata); 
    if (p == NULL)
        return;

    // Elimino posibles caracteres al inicio
    while (*p != ' ')
        p++;

    // Elimino los caracteres en blanco que quedan
    while (*p == ' ')
        p++;
    
    // Decodifico
    while (*p != '\r') {
        c0 = *p++;
        c1 = *p++;
        
        // C0 es numerico ?
        d0 = 0;
        if ( c0 < 58 ) {
            d0 = c0 - '0';
        } else if ( c0 < 71) {
            d0 = c0 - 'A' + 10;
        }
        
        // C1 es numerico ?
        d1 = 0;
        if ( c1 < 58 ) {
            d1 = c1 - '0';
        } else if ( c1 < 71) {
            d1 = c1 - 'A' + 10;
        }
        
        dec = d0*16 + d1;
        lBchar_Put(&lora_decoded_rx_sdata, dec);
        //xprintf_P(PSTR("CHR: %d, %c\r\n"),dec, dec);
    }
    
    //lBchar_print(&lora_decoded_rx_sdata);
    xprintf_P(PSTR("RCVD:[%s]\r\n"), lBchar_get_buffer(&lora_decoded_rx_sdata));
}
//------------------------------------------------------------------------------
void lora_read_snr(void)
{
    
char *p = NULL;

    lBchar_Flush(&lora_tx_sdata);
    lBchar_Flush(&lora_rx_sdata);

    // Envio el comando
    sprintf( lBchar_get_buffer(&lora_tx_sdata), "radio get snr" );
    if (systemConf.debug_lora_comms) {
        xfprintf( fdTERM, "%s\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    }
    xfprintf( fdLORA, "%s\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
    //
    // Leo la respuesta
    p = lBchar_get_buffer(&lora_rx_sdata); 
    systemVars.lora_snr = atoi(p);
    
}
//------------------------------------------------------------------------------
void lora_send_confirmation(void)
{
    /*
     * Si lo que recibi no es OK, mando OK.
     */
    
char *p = NULL;

    //Veo si el mensaje recibido es 'OK'.
    p = lBchar_get_buffer(&lora_decoded_rx_sdata);
    //xprintf_P(PSTR("DEBUG [%s]\r\n"),p);
    if ( strstr( p, "OK") != NULL ) {
        xfprintf( fdTERM, "Rcvd confirmation.\r\n" );
        return;
    }

    lBchar_Flush(&lora_tx_sdata);
    lBchar_Flush(&lora_rx_sdata);

    // Envio la respuesta OK (4F4B)
    sprintf( lBchar_get_buffer(&lora_tx_sdata), "radio tx 4F4B" );
    if (systemConf.debug_lora_comms) {
        xfprintf( fdTERM, "%s\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    }
    xfprintf( fdLORA, "%s\r\n", lBchar_get_buffer(&lora_tx_sdata) );
    
    xfprintf( fdTERM, "Sent confirmation.\r\n" );
    vTaskDelay( ( TickType_t)( 50 / portTICK_PERIOD_MS ) );
    
}
//------------------------------------------------------------------------------
