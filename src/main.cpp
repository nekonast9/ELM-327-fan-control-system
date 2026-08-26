#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>

#include <millis.h>
#include <uart.h>
#include <softuart.h>
#include <adc.h>
#include <elm327.h>

/* --- Clock and PWM settings --- */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#define FAN_FREQ 100
#define PWM_PRESCALER 8
#define PWM_TOP ((F_CPU / (PWM_PRESCALER * FAN_FREQ)) - 1)

/* --- Pin definitions --- */
#define FAN_PIN                PB1 /* D9 (OC1A) */
#define COMPRESSOR_RELAY_PIN   PB4 /* D12 */
#define FAN_COMPRESSOR_ON_IDLE 60 // %
#define FAN_COMPRESSOR_ON_3000 80 // %

/* Settings */
// #define DISPLAY

#define WORK_AFTER_IGNITION_OFF_TIMEOUT_MS 30000 /* Time to keep working after ignition is turned off */
#define IGNITION_OFF_OBD_TIMEOUT_MS        8000  /* Max OBD silence before declaring ignition off */

/* Functions */

void initHardware(void) {
    /* GPIO config */

    /* OUTPUT */
    DDRB |= (1 << FAN_PIN);

    /* INPUT */
    DDRB &= ~(1 << COMPRESSOR_RELAY_PIN);
    PORTB |= (1 << COMPRESSOR_RELAY_PIN);

    /* Timer1: Mode 14 (Fast PWM), TOP = ICR1 */
    /* COM1A1: Non-inverting PWM on OC1A */
    TCCR1A = (1 << COM1A1) | (1 << WGM11);

    /* CS11: Prescaler 8 (matches PWM_PRESCALER) */
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

    ICR1 = PWM_TOP;
    OCR1A = 0;
}

void setFanPower(uint8_t percent) {
    const uint8_t MIN_DC = 10;
    const uint8_t MAX_DC = 100;

    uint8_t inverted_percent;
    if (percent > 100) percent = 100;
    inverted_percent = MAX_DC - (percent * (MAX_DC-MIN_DC) / 100);
    OCR1A = ((uint32_t)inverted_percent * PWM_TOP) / 100;
}

uint8_t calculateFanP(float currentTemp) {
    const float T_LOW  = 96.0;
    const float T_HIGH = 106.0;
    const uint8_t MAX_LIMIT = 100;

    static bool isRunning = false;

    if (!isRunning && currentTemp >= T_LOW) {
        isRunning = true;
    }
    else if (isRunning && currentTemp < (T_LOW - 0.5)) {
        isRunning = false;
    }

    if (!isRunning) {
        return 0;
    }

    if (currentTemp < T_LOW) {
        return 10;
    }

    if (currentTemp >= T_HIGH) {
        return MAX_LIMIT;
    }

    float power = (currentTemp - T_LOW) * (float)MAX_LIMIT / (T_HIGH - T_LOW);
    return (uint8_t)power;
}

bool isCompressorOn(void) {
    return !!(PINB & (1 << COMPRESSOR_RELAY_PIN));
}

typedef enum {
    MODE_AUTO,
    MODE_MANUAL
} fan_mode_t;

fan_mode_t currentMode = MODE_AUTO;
uint8_t manualPower = 0;


int main(void) {
    float jeepTemp = 0.0;
    float last_valid_temp = 0.0;
    uint8_t fanPower = 0;
    bool ignitionOn = false;
    uint32_t last_ignition_time_off = 0;
    uint32_t last_valid_obd_ms = 0;

    initHardware();
    uart_init();
    softuart_init();
    millis_init();

    sei();            /* Enable Global Interrupts */

    while (1) {
        // processNextionInput();

        int16_t temp_raw = elm327_get_coolant_temp();
        int16_t rpm = ELM327_FAIL;
        bool rpm_fetched = false;

        if (temp_raw == ELM327_DISCONNECTED || temp_raw == ELM327_FAIL) {
            printf("ELM327 failed to provide temperature\n");
            jeepTemp = last_valid_temp;

            rpm = elm327_get_rpm();
            rpm_fetched = true;
            printf("RPM: %d\n", rpm);

            if (rpm > 0) {
                last_valid_obd_ms = millis();
                if (!ignitionOn) {
                    printf("Ignition changed to on...\n");
                    ignitionOn = true;
                }
            }

        } else {
            last_valid_temp = (float)temp_raw;
            jeepTemp = last_valid_temp;
            last_valid_obd_ms = millis();
            if (!ignitionOn) {
                printf("Ignition changed to on...\n");
                ignitionOn = true;
            }
        }

        if (ignitionOn && (millis() - last_valid_obd_ms) > IGNITION_OFF_OBD_TIMEOUT_MS) {
            printf("Ignition changed to off...\n");
            last_ignition_time_off = millis();
            ignitionOn = false;
        }

        if (!ignitionOn && (millis() - last_ignition_time_off) < WORK_AFTER_IGNITION_OFF_TIMEOUT_MS) {
            printf("Ignition is off, but still working for %lu ms... Fan: %d%%\n", WORK_AFTER_IGNITION_OFF_TIMEOUT_MS - (millis() - last_ignition_time_off), fanPower);
            _delay_ms(1000);
            continue;
        } else if (!ignitionOn) {
            printf("Ignition is off, stopping fan...\n");
            setFanPower(0);
            _delay_ms(1000);
            continue;
        }

        if (currentMode == MODE_AUTO) {
            fanPower = calculateFanP(jeepTemp);
        } else {
            fanPower = manualPower;
        }

        bool compressor_on = isCompressorOn();
        printf("Compressor: %s\n", compressor_on ? "ON" : "OFF");

        if (!rpm_fetched) {
            rpm = elm327_get_rpm();
            printf("RPM: %d\n", rpm);
        }

        if (compressor_on) {
            if (rpm < 3000) {
                if (fanPower < FAN_COMPRESSOR_ON_IDLE) {
                    fanPower = FAN_COMPRESSOR_ON_IDLE;
                }
            } else {
                if (fanPower < FAN_COMPRESSOR_ON_3000) {
                    fanPower = FAN_COMPRESSOR_ON_3000;
                }
            }
        }

        setFanPower(fanPower);

        printf("Temp: %d, Fan: %d%%, Mode: %s\n",
               (int)jeepTemp, fanPower, (currentMode == MODE_AUTO ? "AUTO" : "MANUAL"));

        if (compressor_on) {
            _delay_ms(1000);
        } else {
            _delay_ms(3000);
        }
    }

    return 0;
}
