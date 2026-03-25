#ifndef SOFTUART_H
#define SOFTUART_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#ifndef F_CPU
    #define F_CPU 16000000UL
#endif

#define SOFTUART_BAUD_RATE      38400

/* RX = PD3 (D3), TX = PD5 (D5) */
#define SOFTUART_RXPIN   PIND
#define SOFTUART_RXDDR   DDRD
#define SOFTUART_RXBIT   PD3

#define SOFTUART_TXPORT  PORTD
#define SOFTUART_TXDDR   DDRD
#define SOFTUART_TXBIT   PD5

#define SOFTUART_T_COMP_LABEL      TIMER2_COMPA_vect
#define SOFTUART_T_COMP_REG        OCR2A
#define SOFTUART_T_CONTR_REGA      TCCR2A
#define SOFTUART_T_CONTR_REGB      TCCR2B
#define SOFTUART_T_CNT_REG         TCNT2
#define SOFTUART_T_INTCTL_REG      TIMSK2

#define SOFTUART_CMPINT_EN_MASK    (1 << OCIE2A)
#define SOFTUART_CTC_MASKA         (1 << WGM21)
#define SOFTUART_CTC_MASKB         (0)

#define SOFTUART_PRESCALE          8
#define SOFTUART_PRESC_MASKA       (0)
#define SOFTUART_PRESC_MASKB       (1 << CS21)

#define SOFTUART_TIMERTOP ( (F_CPU/SOFTUART_PRESCALE/SOFTUART_BAUD_RATE/3) - 1)

#if (SOFTUART_TIMERTOP > 0xff)
    #warning "Check SOFTUART_TIMERTOP: increase prescaler or lower baud rate"
#endif

#define SOFTUART_IN_BUF_SIZE     64

#ifdef __cplusplus
extern "C" {
#endif

void softuart_init(void);
void softuart_flush_input_buffer(void);
unsigned char softuart_kbhit(void);
char softuart_getchar(void);
void softuart_putchar(const char c);
void softuart_puts(const char *s);
void softuart_puts_p(const char *prg_s);

#define softuart_puts_P(s___) softuart_puts_p(PSTR(s___))

#ifdef __cplusplus
}
#endif

#endif /* SOFTUART_H */
