#include "uart.h"

static FILE uart_output = FDEV_SETUP_STREAM((int (*)(char, FILE*))uart_putchar, NULL, _FDEV_SETUP_WRITE);
static FILE uart_input  = FDEV_SETUP_STREAM(NULL, (int (*)(FILE*))uart_getchar, _FDEV_SETUP_READ);

void uart_init(void) {
    UBRR0H = (unsigned char)(UART_CALC_UBRR >> 8);
    UBRR0L = (unsigned char)UART_CALC_UBRR;

    UCSR0B = (1 << TXEN0) | (1 << RXEN0);

    /* Frame format */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    stdout = &uart_output;
    stdin  = &uart_input;
}

void uart_putchar(char c) {
    if (c == '\n') {
        uart_putchar('\r');
    }
    /* Wait for clean transmit buffer*/
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

char uart_getchar(void) {
    /* Wait for data in receive buffer */
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

uint8_t uart_available(void) {
    return (UCSR0A & (1 << RXC0));
}
