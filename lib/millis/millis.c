#include "millis.h"

#include <avr/interrupt.h>

static volatile uint32_t _millis_counter = 0;

ISR(TIMER0_COMPA_vect) {
    _millis_counter++;
}

void millis_init(void) {
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);
    OCR0A = 249;
    TIMSK0 |= (1 << OCIE0A);
    _millis_counter = 0;
}

uint32_t millis(void) {
    uint32_t m;
    uint8_t sreg = SREG;
    cli();
    m = _millis_counter;
    SREG = sreg;
    return m;
}
