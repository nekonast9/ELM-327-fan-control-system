#include "adc.h"

void adc_init(void) {
    /* Set Voltage Reference to AVCC */
    ADMUX = ADC_VREF_AVCC;

    /* Enable ADC and set Prescaler to 128 (16MHz / 128 = 125kHz) */
    /* This is the optimal frequency for 10-bit accuracy */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel) {
    /* Select ADC channel (masking to ensure only bits 0-3 are used) */
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    /* Start conversion */
    ADCSRA |= (1 << ADSC);

    /* Wait for conversion to complete (ADSC becomes 0) */
    while (ADCSRA & (1 << ADSC));

    return ADC;
}
