#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <stdio.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define UART_BAUD 9600
#define UART_CALC_UBRR (F_CPU/16/UART_BAUD-1)

/* Function Prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void uart_init(void);
void uart_putchar(char c);
char uart_getchar(void);
uint8_t uart_available(void);

#ifdef __cplusplus
}
#endif

#endif // UART_H
