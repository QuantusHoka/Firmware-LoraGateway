#include <Arduino.h>
#include <DMC_PinMonitor.h>

DMC_MonitoredPin::DMC_MonitoredPin(const char *name, uint8_t pin, QueueHandle_t *event_queue, int pin_high_message, int pin_low_message, uint32_t monitor_interval, uint8_t mode)
{
    strlcpy(PinName, name, sizeof(PinName));
    Pin = pin;
    Mode = mode;
    EventQueue = event_queue;
    HighMessage = pin_high_message;
    LowMessage = pin_low_message;
    Interval = monitor_interval;
}

void DMC_MonitoredPin::begin(){
    startMonitorTask();
}

void DMC_MonitoredPin::startMonitorTask(){
    xTaskCreatePinnedToCore(task_monitorPinState, PinName , PIN_MONITOR_STACK_DEPTH, this, PIN_MONITOR_TASK_PRIORITY, NULL, 1);
}

void task_monitorPinState(void *params) {
    DMC_MonitoredPin *p = (DMC_MonitoredPin *)params;
    int Msg = 0;
    uint8_t PinHx;
    pinMode(p->Pin, p->Mode);
    PinHx = digitalRead(p->Pin) ? 0b11111111 : 0b00000000;
    log_v("%s at Init: %i", p->PinName, PinHx);
    Msg = PinHx ? p->HighMessage : p->LowMessage;
    log_v("Sending Init Pin Msg: %i", Msg);
    xQueueSendToBack(*p->EventQueue, &Msg, 0U);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(p->Interval));
        PinHx = (PinHx << 1);
        PinHx |= digitalRead(p->Pin);
        if ((PinHx & PIN_HX_MASK) == PIN_HIGH) {PinHx = 0b11111111; Msg = p->HighMessage; xQueueSendToBack(*p->EventQueue, &Msg, 0U);}
        if ((PinHx & PIN_HX_MASK) == PIN_LOW) {PinHx = 0b00000000; Msg = p->LowMessage; xQueueSendToBack(*p->EventQueue, &Msg, 0U);}
    }
    vTaskDelete(NULL);
}