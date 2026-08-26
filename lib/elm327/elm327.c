#include "elm327.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <millis.h>

#define ELM_SEND_P(cmd_str) softuart_puts_P(cmd_str "\r\n")

#define ELM_CHECK_CMD(cmd_str) ({ \
    softuart_flush_input_buffer(); \
    ELM_SEND_P(cmd_str); \
    elm327_wait_for_prompt(ELM327_CMD_TIMEOUT_MS); \
})

static ELM327__STATE_t current_elm327_state = ELM327_STATE_INIT;

static bool elm327_wait_for_prompt(uint16_t timeout_ms) {
    uint32_t start = millis();

    while (millis() - start < timeout_ms) {
        if (softuart_kbhit() && softuart_getchar() == '>') return true;
    }
    return false;
}

static bool elm327_init(void) {
    return (ELM_CHECK_CMD("ATZ")    &&
            ELM_CHECK_CMD("ATE0")   &&
            ELM_CHECK_CMD("ATL1")   &&
            ELM_CHECK_CMD("ATSP2")
           );
}

static void elm327_update_state(void) {
    static uint32_t last_check = 0;

    if (millis() - last_check < 3000) return;
    last_check = millis();

    if (current_elm327_state == ELM327_STATE_INIT) {
        if (elm327_init()) {
            current_elm327_state = ELM327_STATE_READY;
        }
    } else {
        if (!ELM_CHECK_CMD("ATRV")) current_elm327_state = ELM327_STATE_INIT;
    }
}

extern int16_t elm327_get_coolant_temp(void) {
    char buf[ELM327_BUFFER_SIZE];
    char raw_buf[ELM327_BUFFER_SIZE];
    char temp_hex[3] = {0};
    char *ptr = NULL;
    uint8_t i_buf = 0;
    uint8_t i_raw_buf = 0;
    uint32_t start = 0;

    elm327_update_state();

    if (current_elm327_state != ELM327_STATE_READY) {
        printf("[OBD][get_coolant_temp] Error: ELM327 not ready. State: %d\n", current_elm327_state);
        return ELM327_DISCONNECTED;
    }

    softuart_flush_input_buffer();
    softuart_puts_P("0105\r");

    start = millis();

    while (millis() - start < ELM327_CMD_TIMEOUT_MS && i_raw_buf < ELM327_BUFFER_SIZE - 1) {
        if (softuart_kbhit()) {
            char c = softuart_getchar();
            if (c == '>') break;
            if (c != ' ' && c != '\r' && c != '\n' && c != '\0') {
                raw_buf[i_raw_buf++] = c;
            }
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
                buf[i_buf++] = c;
            }
        }
    }
    buf[i_buf] = '\0';
    raw_buf[i_raw_buf] = '\0';

    if (i_raw_buf > 0) {
        printf("[OBD][get_coolant_temp] Raw Data: %s\n", raw_buf);
    } else {
        printf("[OBD][get_coolant_temp] Error: No response (Timeout)\n");
    }

    ptr = strstr(buf, "4105");
    if (ptr) {
        strncpy(temp_hex, ptr + 4, 2);
        temp_hex[2] = '\0';

        int16_t raw_val = (int16_t)strtol(temp_hex, NULL, 16);
        int16_t result = raw_val - 40;

        printf("[OBD][get_coolant_temp] Found 4105! Hex: %s, Dec: %d, Temp: %d C\n", temp_hex, raw_val, result);

        softuart_flush_input_buffer();
        return result;
    }

    printf("[OBD][get_coolant_temp] Error: Header 4105 not found in response\n");

    softuart_flush_input_buffer();
    return ELM327_FAIL;
}

extern int16_t elm327_get_rpm(void) {
    char buf[ELM327_BUFFER_SIZE];
    char raw_buf[ELM327_BUFFER_SIZE];
    char rpm_hex[5] = {0};
    char *ptr = NULL;
    uint8_t i_buf = 0;
    uint8_t i_raw_buf = 0;
    uint32_t start = 0;

    elm327_update_state();

    if (current_elm327_state != ELM327_STATE_READY) {
        printf("[OBD][get_rpm] Error: ELM327 not ready. State: %d\n", current_elm327_state);
        return ELM327_DISCONNECTED;
    }

    softuart_flush_input_buffer();
    softuart_puts_P("010C\r");

    start = millis();

    while (millis() - start < ELM327_CMD_TIMEOUT_MS && i_raw_buf < ELM327_BUFFER_SIZE - 1) {
        if (softuart_kbhit()) {
            char c = softuart_getchar();
            if (c == '>') break;
            if (c != ' ' && c != '\r' && c != '\n' && c != '\0') {
                raw_buf[i_raw_buf++] = c;
            }
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
                buf[i_buf++] = c;
            }
        }
    }
    buf[i_buf] = '\0';
    raw_buf[i_raw_buf] = '\0';

    if (i_raw_buf > 0) {
        printf("[OBD][get_rpm] Raw Data: %s\n", raw_buf);
    } else {
        printf("[OBD][get_rpm] Error: No response (Timeout)\n");
        return ELM327_FAIL;
    }

    ptr = strstr(buf, "410C");
    if (ptr) {
        strncpy(rpm_hex, ptr + 4, 4);
        rpm_hex[4] = '\0';

        uint32_t raw_val = (uint32_t)strtol(rpm_hex, NULL, 16);
        int16_t result = (int16_t)(raw_val / 4);

        printf("[OBD][get_rpm] Found 410C! Hex: %s, Dec: %lu, RPM: %d\n", rpm_hex, raw_val, result);

        return result;
    }

    printf("[OBD][get_rpm] Error: Header 410C not found in response\n");

    softuart_flush_input_buffer();
    return ELM327_FAIL;
}
