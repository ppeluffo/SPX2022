
#include "pines.h"

// -----------------------------------------------------------------------------
void OCOUT_init(void)
{
    // Configura el pin del OC como output
	OCOUT_PORT.DIR |= OCOUT_PIN_bm;	
	CLEAR_OCOUT();
}

// -----------------------------------------------------------------------------
void RELE_K1_init(void)
{
	RELE_K1_PORT.DIR |= RELE_K1_PIN_bm;	
	CLEAR_RELE_K1();
}

// -----------------------------------------------------------------------------
void RELE_K2_init(void)
{
	RELE_K2_PORT.DIR |= RELE_K2_PIN_bm;	
	CLEAR_RELE_K2();
}

// -----------------------------------------------------------------------------
void VSENSORS420_init(void)
{
    // Configura el pin del SENSORS420 como output
	VSENSORS420_PORT.DIR |= VSENSORS420_PIN_bm;	
	CLEAR_VSENSORS420();
}

// ---------------------------------------------------------------------------