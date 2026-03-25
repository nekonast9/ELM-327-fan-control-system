#ifndef ELM327_H
#define ELM327_H

#include <stdbool.h>
#include <stdint.h>
#include <softuart.h>

#define ELM327_FAIL -127
#define ELM327_DISCONNECTED -128

#define ELM327_CMD_TIMEOUT_MS 5000
#define ELM327_BUFFER_SIZE 20

typedef enum {
    ELM327_STATE_INIT,
    ELM327_STATE_READY
} ELM327__STATE_t;

#ifdef __cplusplus
extern "C" {
#endif

extern int16_t elm327_get_coolant_temp(void);

#ifdef __cplusplus
}
#endif

#endif // ELM327_H
