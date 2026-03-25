#ifndef ADC_H
#define ADC_H

#include <avr/io.h>
#include <stdint.h>

/* ADC configuration */
#define ADC_VREF_AVCC (1 << REFS0) /* Use AVCC as reference */

/* Function Prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void adc_init(void);
uint16_t adc_read(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* ADC_H */
