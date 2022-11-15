
#include "dac.h"


//------------------------------------------------------------------------------
void VREF_init(void)
{
    /*
     Para usar le DAC debemos configurar su voltaje de referencia.
     */
    //VREF.DAC0REF = VREF_REFSEL_2V500_gc | VREF_ALWAYSON_bm;
    VREF.DAC0REF = VREF_REFSEL_VREFA_gc | VREF_ALWAYSON_bm;

}
//------------------------------------------------------------------------------
void DAC_init(void)
{
    /* Disable digital input buffer */
    PORTD.PIN6CTRL &= ~PORT_ISC_gm;
    PORTD.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    /* Disable pull-up resistor */
    PORTD.PIN6CTRL &= ~PORT_PULLUPEN_bm;
    /* Enable DAC */
    /* Enable output buffer */
    /* Enable Run in Standby mode */ 
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm | DAC_RUNSTDBY_bm;
   
}
//------------------------------------------------------------------------------
void DAC_setVal(uint16_t value)
{
    // value es de 10 bits o sea que va de 0 a 1023
    
    /* Store the two LSbs in DAC0.DATAL */
    DAC0.DATAL = (value & LSB_MASK) << 6;
    /* Store the eight MSbs in DAC0.DATAH */
    DAC0.DATAH = value >> 2;
}
//------------------------------------------------------------------------------
uint16_t DAC_convertLevel2Value(uint8_t level )
{
    // Toma un valor de level de 0 a 10 y genera el valor de su salida
    // para convertir una corriente
    // 4 mA => 0
    // 20 mA ==> 10
    // El incremento es discreto (0,1,2,...10)
    
uint16_t valor_DAC;
    
    // Convierto el level 0-10 a valores del DAC
    valor_DAC = 1023/10 * level;
    // La corriente de salida va a ser Io=4+16/1023*valor_DAC
    return(valor_DAC);
}
//------------------------------------------------------------------------------
