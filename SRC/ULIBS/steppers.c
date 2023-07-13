
#include "steppers.h"

// https://www.monolithicpower.com/bipolar-stepper-motors-part-i-control-modes

static int8_t phase = 0;

//------------------------------------------------------------------------------
void stepper_init_phase(void)
{
	phase = 2;
}
//------------------------------------------------------------------------------
void stepper_next_phase( t_stepper_dir dir)
{
	// Genera la secuencia que debe aplicar el stepper para moverse en la direccion dir.

	if ( dir == STEPPER_FWD ) {
		phase++;
		if ( phase == 4) {
			phase = 0;
		}

	}

	if ( dir == STEPPER_REV ) {
		phase--;
		if ( phase == -1 ) {
			phase = 3;
		}
	}

}
//------------------------------------------------------------------------------
void stepper_set_phase( uint8_t phase, uint16_t dtime)
{
	// Aplica el pulso al motor y genera la siguiente secuencia
     
	switch (phase) {
	case 0:
		// A+A-
		DRV8814_pulse_Amas_Amenos(dtime);
		break;
	case 1:
		// B+B-
		DRV8814_pulse_Bmas_Bmenos(dtime);
		break;
	case 2:
		// B- 180 degree
        // A-A+
		DRV8814_pulse_Amenos_Amas(dtime);
		break;
	case 3:
		// A-, 90 degree
        // B-B+
		DRV8814_pulse_Bmenos_Bmas(dtime);
		break;
	}

}
//------------------------------------------------------------------------------
void stepper_move( t_stepper_dir dir, uint16_t npulses, uint16_t dtime, uint16_t ptime )
{
	/*
	 * Genera en el stepper una cantidad de pulsos npulses
     * En ancho de c/pulso es dtime.
     * La separacion entre pulsos es ptime
     * separados
	 *
	 */

uint16_t steps;

	// Activo el driver
	DRV8814_awake();
	vTaskDelay( ( TickType_t)( 1000 / portTICK_PERIOD_MS ) );

	// Pongo la secuencia incial en 2 para que puede moverme para adelante o atras
	// sin problemas de incializacion
	//stepper_init_sequence();
	for (steps=0; steps < npulses; steps++) {
		stepper_set_phase(phase, dtime);
        stepper_next_phase(dir);
        vTaskDelay( ( TickType_t)( ptime / portTICK_PERIOD_MS ) );
        xprintf_P(PSTR("p(%d) %03d,\r\n"), phase, steps);
	}
	// Desactivo el driver
	DRV8814_sleep();
}
//------------------------------------------------------------------------------
bool stepper_test( char *s_cmd, char *s_dir, char *s_npulses, char *s_dtime, char *s_ptime)
{
    
t_stepper_dir dir;
uint16_t npulses = atoi(s_npulses);
uint16_t dtime = atoi(s_dtime);
uint16_t ptime = atoi(s_ptime);


    if (!strcmp_P( strupr(s_cmd), PSTR("AWAKE"))) {
        DRV8814_awake();
        return(true);
    }

    if (!strcmp_P( strupr(s_cmd), PSTR("SLEEP"))) {
        DRV8814_sleep();
        return(true);
    }

    if (!strcmp_P( strupr(s_cmd), PSTR("PHA01"))) {
        DRV8814_pulse_Amenos_Amas(0);
        return(true);
    }

    if (!strcmp_P( strupr(s_cmd), PSTR("PHA10"))) {
        DRV8814_pulse_Amas_Amenos(0);
        return(true);
    }

    if (!strcmp_P( strupr(s_cmd), PSTR("PHB01"))) {
        DRV8814_pulse_Bmenos_Bmas(0);
        return(true);
    }

    if (!strcmp_P( strupr(s_cmd), PSTR("PHB10"))) {
        DRV8814_pulse_Bmas_Bmenos(0);
        return(true);
    }

    if (!strcmp_P( strupr(s_cmd), PSTR("MOVE"))) {
     
        if (!strcmp_P( strupr(s_dir), PSTR("FW"))) {
            dir = STEPPER_FWD;
        } else if (!strcmp_P( strupr(s_dir), PSTR("REV"))) {
            dir = STEPPER_REV;
        } else {
            return (false);  
        }
    
        xprintf_P(PSTR("Steppers: dir=%d, pulsos=%d, pw=%d, T=%d\r\n"), dir, npulses, dtime,ptime);
        stepper_move( dir, npulses, dtime, ptime );
        return (true);
    }
    
    return(false);

}
//------------------------------------------------------------------------------
