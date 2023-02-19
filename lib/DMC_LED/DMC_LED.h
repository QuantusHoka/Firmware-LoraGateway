/*
1. Declare a const array of leds.
    const DMC_LED_t array_of_leds[] = {{LED1 stuff}, {LED2 Stuff}, ...};
2. Pass array to a new DMC_LED_CONTROL instance
3. Use public member functions to manipulate LED function


add const to the LED_CONTROL constrctor
for now, the LED controller only allows 8 pwm leds
*/


#ifndef DMC_LED_H
#define DMC_LED_H

#include <Arduino.h>

//LED Defaults
#define LED_PWM_RESOLUTION 8U
#define LED_DEFAULT_DIM_SETTING 128U
#define LED_DEFAULT_BRIGHT_SETTING 255U
#define LED_DEFAULT_PULSE_LENGTH 500U //ms
#define LED_UPDATE_INTERVAL 10U //ms
#define LED_MAX_PWM_LEDS 8
#define LED_ONOFF_PIN_CHANNEL 255
#define TIMER_TIMEOUT 1000 //ms

typedef enum {
    led_mode_off,
    led_mode_on,
    led_mode_blink,
    led_mode_dim,
    led_mode_bright,
    led_mode_dim_blink,
    led_mode_bright_blink,
    led_mode_pulse
} LedMode_t;

typedef enum {
    led_state_off,
    led_state_dim,
    led_state_bright,
    led_state_on
} LedState_t;

typedef enum {
    led_type_onoff,
    led_type_pwm
} LedType_t;

typedef enum {
    led_OK = 0,
    led_error_too_many_pwm,
    led_error_timer_create_failed,
    led_invalid_mode_command
} LedError_t;

struct DMC_Led{
    uint8_t pin;
    LedType_t type;
    bool inverted;
};

class DMCLedController
{
    public:
    DMCLedController(const char *t_name, DMC_Led *t_leds);
    ~DMCLedController(void);
    BaseType_t begin(void);
    LedError_t update(void);
    LedError_t setBright(int t_id, int t_brightSet);
    LedError_t setDim(int t_id, int t_dimSet);
    LedError_t setMode(int t_id, LedMode_t t_mode);
    LedError_t setOnTime(int t_id, uint32_t t_onTime);
    LedError_t setOffTime(int t_id, uint32_t t_offTime);
    TimerHandle_t getTimerHandle(void);

    private:
    DMC_Led *m_led;
    int m_numLeds;
    TimerHandle_t m_timerHandle;
    LedError_t m_err;
    uint8_t *m_channel;
    uint8_t *m_brightSet;
    uint8_t *m_dimSet;
    uint8_t *m_on;
    uint8_t *m_off;
    TickType_t *m_onTime;
    TickType_t *m_offTime;
    TickType_t *m_nextChangeTickCount;
    uint8_t *m_mode;
    uint8_t *m_state;

};

void ledTimerCallback (TimerHandle_t t_xTimer);
#endif