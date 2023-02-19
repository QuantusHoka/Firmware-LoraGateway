/*
These processes are used for "click detection" (single, double, long)
They are intended to be suitable for most uses; some uses wull require custom implementations

In general, use is as follows:
1. Set-up an enum for button events (pressed and released) - the enum object should
have different items for each button pressed and released event (e.g. B1P, B1R, B2P, B2R).
These events will be sent to an instance of the ButtonProcessor Class.

2. Set-up the queues that will receive events from the button processor (e.g. system event queue)

3. Set-up the enumerated events that will be sent to that queue including singlePress, doublePress
and longPress events.

3. Create an array of DMCButton objects, including for each button the pressed/released events
that will be passed to the button processor queue, the queue to which processed events from
the button processor will be sent, and the enumerated events for single, double and long press events.

4. Instantiate the button processor with the array of button objects, above.

5. Use either IRQs, or the PinMonitor class to send pressed/released events to the button processor.
*/



#ifndef DMC_BUTTON_H
#define DMC_BUTTON_H

#include <Arduino.h>

#define BUTTON_PROCESSOR_TASK_STACK_DEPTH 0x600
#define BUTTON_PROCESSOR_TASK_PRIORITY 8U
#define BUTTON_PROCESSOR_EVENT_TIME 500U
#define BUTTON_PROCESSOR_NO_EVENT_TIME 60000U
#define BUTTON_PIN_MONITOR_INTERVAL 5U

struct DMCButton {
  uint8_t inPressedCode;
  uint8_t inReleasedCode;
  QueueHandle_t *outEventQueue;
  uint8_t outSingleCode;
  uint8_t outDoubleCode;
  uint8_t outLongCode;
};

class DMCButtonProcessor
{
  public:
  DMCButtonProcessor(const char *t_name, DMCButton *t_buttons);
  ~DMCButtonProcessor(void);
  QueueHandle_t *getInQueue(void);
  int getNumButtons(void);
  uint8_t getSingleClickMessage(int t_id);
  uint8_t getDoubleClickMessage(int t_id);
  uint8_t getLongClickMessage(int t_id);
  QueueHandle_t *getOutQueue(int t_id);
  uint8_t getInPressedCode(int t_id);
  uint8_t getInReleasedCode(int t_id);

  void begin(void);

  private:
  QueueHandle_t m_buttonQueue;
  DMCButton *m_button;
  char m_name[16];
  int m_numButtons;
  TickType_t *m_eventEnd;
  int *m_pressed;
  int *m_released;
};

void buttonProcessorTask(void *params);

#endif