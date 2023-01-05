#include <Arduino.h>
#include <DMC_Button.h>
#include <DMC_PinMonitor.h>

DMC_Button::DMC_Button(const char *n, uint8_t pin, QueueHandle_t *system_event_queue, int long_press_event_code, int single_press_event_code, int double_press_event_code)
{
  strlcpy(Name, n, sizeof(Name));
  Pin = pin;
  SystemEventQueue = system_event_queue;
  LongPressEvent = long_press_event_code;
  SingleClickEvent = single_press_event_code;
  DoubleClickEvent = double_press_event_code;
  ButtonEventQueue = xQueueCreate(8U, sizeof(ButtonEvent_t));
}

void DMC_Button::begin(void){
  xTaskCreatePinnedToCore( task_buttonProcessor, Name, BUTTON_PROCESSOR_TASK_STACK_DEPTH , this, BUTTON_PROCESSOR_TASK_PRIORITY, NULL, 1);
}

void task_buttonProcessor(void *params)
{
  DMC_Button *b = (DMC_Button*) params;
  ButtonEvent_t MonitoredPinEvent;
  int system_event_message;
  TickType_t EventStart = 0;
  uint8_t released = 0;
  uint8_t pressed = 0;

  DMC_MonitoredPin MonitoredPin(b->Name, b->Pin, &b->ButtonEventQueue, button_event_Pressed, button_event_Released, BUTTON_PIN_MONITOR_INTERVAL, INPUT);
  MonitoredPin.begin();

  while (1) {
    xQueueReceive(b->ButtonEventQueue, &MonitoredPinEvent, portMAX_DELAY);
    if (MonitoredPinEvent == button_event_Pressed) {
      EventStart = xTaskGetTickCount();
      pressed = 1;
      released = 0;
      while (xTaskGetTickCount() <= (EventStart + BUTTON_PROCESSOR_EVENT_TIME)) {
        if(xQueueReceive(b->ButtonEventQueue, &MonitoredPinEvent, pdMS_TO_TICKS(10))) {
          released += (MonitoredPinEvent == button_event_Released);
          pressed += (MonitoredPinEvent == button_event_Pressed);
        }
      }
      if (pressed > 1) {system_event_message = b->DoubleClickEvent; log_v("Double click");}
      if (released == 0) {system_event_message = b->LongPressEvent; log_v("Long press");}
      if ((pressed == 1) && (released == 1)) {system_event_message = b->SingleClickEvent; log_v("Single click");}
      xQueueSendToBack(*b->SystemEventQueue, &system_event_message, 0U);
    }
  }
  vTaskDelete(NULL);
}