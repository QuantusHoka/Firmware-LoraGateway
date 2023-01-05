//Create a Monitored Pin Instance - define queue and simple enum message (must be Queue of Int) (and pin number and name and polling interval)
//Create Task
//Done - messages will be passed to queue when event happens

#ifndef DMC_PINMONITOR_H
#define DMC_PINMONITOR_H

#include <Arduino.h>

#define PIN_HX_MASK 0b11111111
#define PIN_HIGH 0b00001111
#define PIN_LOW  0b11100000
#define PIN_MONITOR_STACK_DEPTH 0X700
#define PIN_MONITOR_TASK_PRIORITY 10U
#define PIN_MONITOR_DEFAULT_INTERVAL 100U
#define PIN_MONITOR_DEFAULT_MODE INPUT_PULLUP
//#define PIN_MONITOR_HIGHWATER_REPORT_INTERVAL 1000

class DMC_MonitoredPin {
    public:
    char PinName[16];
    uint8_t Pin;
    uint8_t Mode;
    QueueHandle_t *EventQueue;
    int HighMessage, LowMessage;
    uint32_t Interval;

    DMC_MonitoredPin(const char *name, uint8_t pin = GPIO_NUM_1, QueueHandle_t *event_queue = NULL, int pin_high_message = 0x01, int pin_low_message = 0x00, uint32_t monitor_interval = PIN_MONITOR_DEFAULT_INTERVAL, uint8_t mode = PIN_MONITOR_DEFAULT_MODE);
    void begin(void);

    private:
    void startMonitorTask();

};


void task_monitorPinState(void *params);
#endif