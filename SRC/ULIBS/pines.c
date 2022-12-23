
#include "pines.h"

// -----------------------------------------------------------------------------

void OCOUT_init(void)
{
    // Configura el pin del OC como output
	OCOUT_PORT.DIR |= OCOUT_PIN_bm;	
	CLEAR_OCOUT();
}

// -----------------------------------------------------------------------------
void VSENSORS420_init(void)
{
    // Configura el pin del SENSORS420 como output
	VSENSORS420_PORT.DIR |= VSENSORS420_PIN_bm;	
	CLEAR_VSENSORS420();
}

// ---------------------------------------------------------------------------