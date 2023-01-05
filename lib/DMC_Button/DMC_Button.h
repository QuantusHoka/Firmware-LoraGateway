/*
Create Button Event Queue
Create monitored pin instance for the button
Create a button instance for the button

*/

#ifndef DMC_BUTTON_H
#define DMC_BUTTON_H

#include <Arduino.h>
#include <DMC_PinMonitor.h>

#define BUTTON_PROCESSOR_TASK_STACK_DEPTH 0x600
#define BUTTON_PROCESSOR_TASK_PRIORITY 8U
#define BUTTON_PROCESSOR_EVENT_TIME 500U
#define BUTTON_PIN_MONITOR_INTERVAL 5U


#define createButtonProcessorTask(ButtonMonitorTaskName, ButtonObjectInstance) xTaskCreate( task_buttonProcessor, ( ButtonMonitorTaskName ), BUTTON_PROCESSOR_TASK_STACK_DEPTH , ( ButtonObjectInstance ), BUTTON_PROCESSOR_TASK_PRIORITY, NULL)

typedef enum {
    button_event_Pressed,
    button_event_Released
} ButtonEvent_t;

struct DMC_Button
{
  char Name[16];
  uint8_t Pin;
  QueueHandle_t ButtonEventQueue;
  QueueHandle_t *SystemEventQueue;
  int LongPressEvent, SingleClickEvent, DoubleClickEvent;

  DMC_Button(const char *n, uint8_t pin, QueueHandle_t *system_event_queue, int long_press_event_code, int single_press_event_code, int double_press_event_code);

  void begin(void);

};

void task_buttonProcessor(void *params);

#endif