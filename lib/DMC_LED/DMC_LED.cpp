#include <Arduino.h>
#include <DMC_LED.h>

DMC_LED::DMC_LED(const char *n, uint8_t p, uint8_t c){
  Pin = p;
  Channel = c;
  Dim = true;
  LongPulse = true;
  temp = false;
  operation = Led_state_off;
  LEDDim = 128U;
  LEDBright = 255U;
  strlcpy(Name, n, sizeof(Name));
  ShortTimerHandle = xTimerCreate("ShortPulseTimer", pdMS_TO_TICKS(LED_PULSE_SHORT), pdFALSE, ( void * ) &EventQueue, timer_DMCLedCallback);
  LongTimerHandle = xTimerCreate("LongPulseTimer", pdMS_TO_TICKS(LED_PULSE_LONG), pdFALSE, ( void * ) &EventQueue, timer_DMCLedCallback);
  EventQueue = xQueueCreate(10U, sizeof(LedEvent_t));
}

void DMC_LED::begin()
{
  xTaskCreatePinnedToCore(task_DMCLedControl, Name, LED_CONTROL_STACK_DEPTH, this, LED_CONTROL_TASK_PRIORITY, NULL, 1);
}

void DMC_LED::dispatch(LedEvent_t const *e){
  uint32_t v = e->value;
  v = (v < 0) ? 0 : (v > 255) ? 255 : v;
  switch (e->code){
    case Led_event_timerexpired:
      if (LongPulse){
        ledcWrite(Channel, Dim ? LEDBright : LEDDim);
        LongPulse = false;
        xTimerStart(ShortTimerHandle, pdMS_TO_TICKS(100));
      } else {
        ledcWrite(Channel, Dim ? LEDDim : LEDBright);
        LongPulse = true;
        xTimerStart(LongTimerHandle, pdMS_TO_TICKS(100));
      }
      break;

      case Led_set_dim:
      log_v("led set day dim val: %i", v);
      LEDDim = v;
      break;

    case Led_set_bright:
      log_v("led set day bright val: %i", v);
      LEDBright = v;
      break;

    case Led_state_off:
    log_v("Led_state_off");
      operation = Led_state_off;
      if (xTimerIsTimerActive(ShortTimerHandle)) {xTimerStop(ShortTimerHandle, pdMS_TO_TICKS(100));}
      if (xTimerIsTimerActive(LongTimerHandle)) {xTimerStop(LongTimerHandle, pdMS_TO_TICKS(100));}
      ledcWrite(Channel, 0U);
      break;

    case Led_state_dim:
    log_v("Led_state_dim");
      operation = Led_state_dim;
      if (xTimerIsTimerActive(ShortTimerHandle)) {xTimerStop(ShortTimerHandle, pdMS_TO_TICKS(100));}
      if (xTimerIsTimerActive(LongTimerHandle)) {xTimerStop(LongTimerHandle, pdMS_TO_TICKS(100));}
      ledcWrite(Channel, LEDDim);
      Dim = true;
      break;

    case Led_state_bright:
      log_v("Led_state_bright");
      operation = Led_state_bright;
      if (xTimerIsTimerActive(ShortTimerHandle)) {xTimerStop(ShortTimerHandle, pdMS_TO_TICKS(100));}
      if (xTimerIsTimerActive(LongTimerHandle)) {xTimerStop(LongTimerHandle, pdMS_TO_TICKS(100));}
      ledcWrite(Channel, LEDBright);
      Dim = false;
      break;

    case Led_state_dimpulse:
      log_v("Led_state_dimpulse");
      operation = Led_state_dimpulse;
      if (xTimerIsTimerActive(ShortTimerHandle)) {xTimerStop(ShortTimerHandle, pdMS_TO_TICKS(100));}
      if (xTimerIsTimerActive(LongTimerHandle)) {xTimerStop(LongTimerHandle, pdMS_TO_TICKS(100));}
      ledcWrite(Channel, LEDDim);
      Dim = true; LongPulse = true;
      xTimerStart(LongTimerHandle, pdMS_TO_TICKS(50));
      break;

    case Led_state_brightpulse:
      log_v("Led_state_brightpulse");
      operation = Led_state_brightpulse;
      if (xTimerIsTimerActive(ShortTimerHandle)) {xTimerStop(ShortTimerHandle, pdMS_TO_TICKS(100));}
      if (xTimerIsTimerActive(LongTimerHandle)) {xTimerStop(LongTimerHandle, pdMS_TO_TICKS(100));}
      ledcWrite(Channel, LEDBright);
      Dim = false; LongPulse = true;
      xTimerStart(LongTimerHandle, pdMS_TO_TICKS(50));
      break;

    case Led_state_tempset:
      log_v("Led_state_tempset");
      temp = true;
      if (xTimerIsTimerActive(ShortTimerHandle)) {xTimerStop(ShortTimerHandle, pdMS_TO_TICKS(100));}
      if (xTimerIsTimerActive(LongTimerHandle)) {xTimerStop(LongTimerHandle, pdMS_TO_TICKS(100));}
      ledcWrite(Channel, v);
      break;

    case Led_event_resume:
      log_v("Led_event_resume");
      LedEvent_t le;
      temp = false;
      le.code = operation; le.value = 0;
      xQueueSendToBack(EventQueue, &le, 0U);
  }
}

void timer_DMCLedCallback(TimerHandle_t xtimer)
{
    QueueHandle_t *q = (QueueHandle_t *) pvTimerGetTimerID(xtimer);
    LedEvent_t e(Led_event_timerexpired);
    xQueueSendToBack(*q, &e, 0U);
}

void task_DMCLedControl(void *params) {
  DMC_LED *led = (DMC_LED *)params;
  LedEvent_t LEDEvent;

  ledcSetup(led->Channel, 5000U, LED_PWM_RESOLUTION);
  ledcAttachPin(led->Pin, led->Channel);
  ledcWrite(led->Channel, 0U);
  while (1) {
    log_v("High water: %i", uxTaskGetStackHighWaterMark(NULL));
    xQueueReceive(led->EventQueue, &LEDEvent, portMAX_DELAY);
    led->dispatch(&LEDEvent);
  }
  vTaskDelete(NULL);
}