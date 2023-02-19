#ifndef DMC_PINMONITOR_H
#define DMC_PINMONITOR_H

#include <Arduino.h>

#define PIN_HX_MASK     0b1111100000011111
#define PIN_HIGH        0b0000000000011111
#define PIN_LOW         0b1111100000000000
#define PIN_HIGH_SET    0b1111111111111111
#define PIN_LOW_SET     0b0000000000000000
#define PIN_MONITOR_STACK_DEPTH 0X700
#define PIN_MONITOR_TASK_PRIORITY 10U
#define PIN_MONITOR_INTERVAL 16U
#define PIN_MONITOR_DEFAULT_MODE INPUT_PULLUP
#define TIMER_TIMEOUT 1000 //ms
//#define PIN_MONITOR_HIGHWATER_REPORT_INTERVAL 1000

struct DMC_Pin {
    uint8_t pinNumber;
    uint8_t mode;
    QueueHandle_t *eventQueue;
    int highMessage, lowMessage;
};

class DMCPinMonitor {
    public:
    DMCPinMonitor(const char *t_name, DMC_Pin *t_pins);
    ~DMCPinMonitor(void);
    BaseType_t begin(void);
    TimerHandle_t getTimerHandle(void);
    bool update(void);

    private:
    DMC_Pin* m_pin;
    int m_numPins;
    TimerHandle_t m_timerHandle;
    uint16_t *m_hx;
};

void pinTimerCallback(TimerHandle_t t_xtimer);

#endif