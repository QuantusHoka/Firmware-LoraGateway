#include <Arduino.h>
#include <DMC_Button.h>

DMCButtonProcessor::DMCButtonProcessor(const char *t_name, DMCButton *t_buttons){
  strlcpy(m_name, t_name, sizeof(m_name));
  m_numButtons = sizeof(t_buttons) / sizeof(DMCButton);
  m_button = t_buttons;

  m_buttonQueue = xQueueCreate(10U, sizeof(uint8_t));
}

DMCButtonProcessor::~DMCButtonProcessor(void){

  vQueueDelete(m_buttonQueue);
}

void DMCButtonProcessor::begin(void){
  xTaskCreatePinnedToCore( buttonProcessorTask, m_name, BUTTON_PROCESSOR_TASK_STACK_DEPTH , this, BUTTON_PROCESSOR_TASK_PRIORITY, NULL, 1);
}

QueueHandle_t *DMCButtonProcessor::getInQueue(void) {
  return &m_buttonQueue;
}

int DMCButtonProcessor::getNumButtons(void) {
  return m_numButtons;
}

uint8_t DMCButtonProcessor::getSingleClickMessage(int t_id){
  return m_button[t_id].outSingleCode;
}

uint8_t DMCButtonProcessor::getDoubleClickMessage(int t_id){
  return m_button[t_id].outDoubleCode;
}

uint8_t DMCButtonProcessor::getLongClickMessage(int t_id){
  return m_button[t_id].outLongCode;
}

QueueHandle_t *DMCButtonProcessor::getOutQueue(int t_id){
  return m_button[t_id].outEventQueue;
}

  uint8_t DMCButtonProcessor::getInPressedCode(int t_id){
    return m_button[t_id].inPressedCode;
}
  uint8_t DMCButtonProcessor::getInReleasedCode(int t_id){
    return m_button[t_id].inReleasedCode;
}


void buttonProcessorTask(void *params)
{
  DMCButtonProcessor *bm = (DMCButtonProcessor *) params;
  int event;
  int numButtons = bm->getNumButtons();
  TickType_t nextTimeout = portMAX_DELAY;
  TickType_t nextEventEnd = portMAX_DELAY;
  TickType_t eventEnd[numButtons];
  uint8_t pressed[numButtons];
  uint8_t released[numButtons];
  int msg;

  while (1) {
    nextEventEnd = xTaskGetTickCount() + pdMS_TO_TICKS(BUTTON_PROCESSOR_NO_EVENT_TIME);
    if (!xQueueReceive(*bm->getInQueue(), &event, nextTimeout)) { //some event ended
      for (int idx = 0; idx < numButtons; idx++){
        if (xTaskGetTickCount() >= eventEnd[idx]){
          msg = 255;
          if (pressed[idx] > 1) {msg = bm->getDoubleClickMessage(idx); log_v("Double click");}
          if (released[idx] == 0) {msg = bm->getLongClickMessage(idx); log_v("Long press");}
          if ((pressed[idx] == 1) && (released[idx] == 1)) {msg = bm->getSingleClickMessage(idx); log_v("Single click");}
          pressed[idx] = 0;
          eventEnd[idx] = xTaskGetTickCount() + pdMS_TO_TICKS(BUTTON_PROCESSOR_NO_EVENT_TIME);
          xQueueSendToBack(*bm->getOutQueue(idx), &msg, 0U);
        }
        nextEventEnd = (nextEventEnd <= eventEnd[idx]) ? nextEventEnd : eventEnd[idx];
      }
    } else { //new event received
      for (int idx = 0; idx < numButtons; idx++){
        if(event == bm->getInPressedCode(idx)){
          if (pressed[idx] == 0){
            released[idx] = 0;
            eventEnd[idx] = xTaskGetTickCount() + pdMS_TO_TICKS(BUTTON_PROCESSOR_EVENT_TIME);
          }
          pressed[idx]++;
        } else {
          if (pressed[idx] == 0){released[idx] = 0;}
          else {released[idx]++;}
        }
        nextEventEnd = (nextEventEnd <= eventEnd[idx]) ? nextEventEnd : eventEnd[idx];
      }
    }
    nextTimeout = nextEventEnd - xTaskGetTickCount();
  }
  vTaskDelete(NULL);
}