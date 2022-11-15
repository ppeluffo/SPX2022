
#include "pines.h"

// -----------------------------------------------------------------------------

void OCOUT_init(void)
{
    // Configura el pin del OC como output
	OCOUT_PORT.DIR |= OCOUT_PIN_bm;	
	CLEAR_RELEOUT();
}

// -----------------------------------------------------------------------------
void VSENSOR_init(void)
{
    // Configura el pin del VSENSOR como output
	VSENSOR_PORT.DIR |= VSENSOR_PIN_bm;	
	CLEAR_VSENSOR();
}

// -----------------------------------------------------------------------------
void SENSORS420_init(void)
{
    // Configura el pin del SENSORS420 como output
	SENSORS420_PORT.DIR |= SENSORS420_PIN_bm;	
	CLEAR_SENSORS420();
}

// ---------------------------------------------------------------------------