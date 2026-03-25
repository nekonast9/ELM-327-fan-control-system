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
#define FAN_PIN             PB1 /* D9 (OC1A) */
#define IGNITION_PIN        PD5 /* D5 */
#define TEMP_SENSOR_CHANNEL 2   /* ADC0 (A0) */

/* Settings */
// #define ADC_MODE
#define ELM_MODE
// #define DISPLAY

#ifdef ADC_MODE
#define TEMPERATURE_TRIGGER_CORRECTION 5   /* Adjust calculated temperature by this value (positive or negative) */
#endif
#define WORK_AFTER_IGNITION_OFF_TIMEOUT_MS 30000 /* Time to keep working after ignition is turned off */


/* Functions */

void initHardware(void) {
    /* GPIO config */

    /* OUTPUT */
    DDRB |= (1 << FAN_PIN);

    /* INPUT */
    DDRD &= ~(1 << IGNITION_PIN);
    PORTD |= (1 << IGNITION_PIN);

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

float calculateJeepTemp(int adcValue) {
  const float a = 0.0001651;
  const float b = -0.2524;
  const float c = 142.61;

  /* T = a*x^2 + b*x + c */
  float temp = (a * adcValue * adcValue) + (b * adcValue) + c;

  return temp;
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

typedef enum {
    MODE_AUTO,
    MODE_MANUAL
} fan_mode_t;

fan_mode_t currentMode = MODE_AUTO;
uint8_t manualPower = 0;

char cmdBuffer[32];
uint8_t cmdIndex = 0;

int main(void) {
#ifdef ADC_MODE
    uint16_t adc_raw_temp = 0;
#endif
    float jeepTemp = 0.0;
    uint8_t fanPower = 0;
    bool ignitionOn = false;
    size_t last_ignition_time_off = 0;

    initHardware();
    uart_init();
    softuart_init();
    millis_init();
#ifdef ADC_MODE
    adc_init();
#endif

    sei();            /* Enable Global Interrupts */

    while (1) {
        // processNextionInput();

#ifdef ADC_MODE
        adc_raw_temp = adc_read(TEMP_SENSOR_CHANNEL);

        if(!(PIND & (1 << IGNITION_PIN))) {
            if (!ignitionOn) {
                printf("Ignition changed to on...\n");
                ignitionOn = true;
            }
        } else {
            if (ignitionOn) {
                printf("Ignition changed to off...\n");
                last_ignition_time_off = millis();
                ignitionOn = false;
            }
        }

        ignitionOn = true;
#endif

#ifdef ELM_MODE
        jeepTemp = elm327_get_coolant_temp();

        if (jeepTemp == ELM327_DISCONNECTED || jeepTemp == ELM327_FAIL) {
            printf("ELM327 failed to provide temperature\n");
            jeepTemp = 0.0;

            if (ignitionOn) {
                printf("Ignition changed to off...\n");
                last_ignition_time_off = millis();
                ignitionOn = false;
            }

        } else {
            if (!ignitionOn) {
                printf("Ignition changed to on...\n");
                ignitionOn = true;
            }
        }
#endif

        if (!ignitionOn && (millis() - last_ignition_time_off) < WORK_AFTER_IGNITION_OFF_TIMEOUT_MS) {
            printf("Ignition is off, but still working for %lu ms...\n", WORK_AFTER_IGNITION_OFF_TIMEOUT_MS - (millis() - last_ignition_time_off));
            _delay_ms(1000);
            continue;
        } else if (!ignitionOn) {
            printf("Ignition is off, stopping fan...\n");
            setFanPower(0);
            _delay_ms(1000);
            continue;
        }

#ifdef ADC_MODE
        jeepTemp = calculateJeepTemp(adc_raw_temp);
        jeepTemp += TEMPERATURE_TRIGGER_CORRECTION;
#endif

        if (currentMode == MODE_AUTO) {
            fanPower = calculateFanP(jeepTemp);
        } else {
            fanPower = manualPower;
        }

        setFanPower(fanPower);

    #ifdef DISPLAY
        printf("num_temp.val=%d\xFF\xFF\xFF", (int)jeepTemp);
        printf("num_karlson.val=%d\xFF\xFF\xFF", (int)fanPower);
    #endif

#ifdef ADC_MODE
        printf("Temp: %d, Fan: %d%%, Mode: %s, ADC=%u\n",
               (int)jeepTemp, fanPower, (currentMode == MODE_AUTO ? "AUTO" : "MANUAL"), adc_raw_temp);
#endif

#ifdef ELM_MODE
        printf("Temp: %d, Fan: %d%%, Mode: %s\n",
               (int)jeepTemp, fanPower, (currentMode == MODE_AUTO ? "AUTO" : "MANUAL"));
#endif

        _delay_ms(3000);
    }

    return 0;
}
