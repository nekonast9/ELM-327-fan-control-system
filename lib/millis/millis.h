#ifndef MILLIS_H
#define MILLIS_H

#include <avr/io.h>

#ifdef __cplusplus
extern "C" {
#endif

void millis_init(void);
uint32_t millis(void);

#ifdef __cplusplus
}
#endif

#endif