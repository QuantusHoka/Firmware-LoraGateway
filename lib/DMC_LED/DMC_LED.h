/* TODO before putting into production
    1. Remove requirement for task handles, as these are not required for normal use
*/


/* LED Library using FreeRTOS.
    Allows simple control of LEDs by passing simple commands into the LED command queue
    Use
    1. Globals
        a. Create a static instance of a dmcLed_t object
         dmcLed_t(NAME, GPIO_NUM, dmcLedType, PWM Channel, inverted);

    2. Setup
        a. Create the task by calling DMCLedTaskCreate( instance.name, &instance, &instance.taskHandle)
    3. When passing data in the queue:
        a. Create an instance of DMCLedCommand
        b. Set DMCLedCommand instance stuff:
            1. Command code. One of: DMCLedCommandOn, DMCLedCommandOff, DMCLedCommandBlink, DMCLedCommandPWM
            2. Value: Number of blinks, or dutycycle percent (0-100). Ignored if LED type is on/off
        Send commands to dmcLed_t_instance.inputQueue 
*/

#ifndef DMC_LED_H
#define DMC_LED_H

#include <Arduino.h>

//LED Defaults
#define LED_PWM_RESOLUTION 8U
#define LED_PULSE_LONG 2850 //ms
#define LED_PULSE_SHORT 150 //ms
#define LED_DEFAULT_DIM 128U
#define LED_DEFAULT_BRIGHT 255U

#define LED_CONTROL_STACK_DEPTH 0x800
#define LED_CONTROL_TASK_PRIORITY 4U

typedef enum {
    Led_state_off,
    Led_state_dim,
    Led_state_bright,
    Led_state_dimpulse,
    Led_state_brightpulse,
    Led_state_tempset,
    Led_event_timerexpired,
    Led_event_resume,
    Led_set_dim,
    Led_set_bright
} LedOperation_t;

struct LedEvent_t {
    LedOperation_t code;
    uint32_t value;

    LedEvent_t(LedOperation_t o = Led_state_off, uint32_t v = 0U): code(o), value(v){}
};

struct DMC_LED
{
    char Name[16];
    uint8_t Pin;
    uint8_t Channel;
    bool Dim, LongPulse, temp;
    LedOperation_t operation;
    uint32_t LEDDim, LEDBright;
    TimerHandle_t ShortTimerHandle, LongTimerHandle;
    QueueHandle_t EventQueue;

    DMC_LED(const char* n, uint8_t p, uint8_t c);
    void begin();
    void dispatch(LedEvent_t const *e);
};

void timer_DMCLedCallback(TimerHandle_t xtimer);

void task_DMCLedControl(void *params);

#endif