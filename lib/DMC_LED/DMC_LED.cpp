#include <Arduino.h>
#include <DMC_LED.h>

DMCLedController::DMCLedController(const char *t_name, DMC_Led *t_leds){
  int numPWMLeds = 0;
  int channel = 0;
  m_err = LedError_t::led_OK;
  m_timerHandle = nullptr;
  m_numLeds = sizeof(t_leds) / sizeof(DMC_Led);
  m_led = t_leds;
  for(int idx = 0; idx<m_numLeds; idx++){
    if (m_led[idx].type == led_type_pwm) {numPWMLeds++;}
  }

  if (numPWMLeds > LED_MAX_PWM_LEDS){
    m_err = LedError_t::led_error_too_many_pwm; 
    log_d("Too Many PWM LEDs: %d",numPWMLeds);
    return;
  }

  m_channel = new uint8_t[m_numLeds];
  m_brightSet = new uint8_t[m_numLeds];
  m_dimSet = new uint8_t[m_numLeds];
  m_off = new uint8_t[m_numLeds];
  m_on = new uint8_t[m_numLeds];
  m_onTime = new uint32_t[m_numLeds];
  m_offTime = new uint32_t[m_numLeds];
  m_nextChangeTickCount = new uint32_t[m_numLeds];
  m_mode = new uint8_t[m_numLeds];
  m_state = new uint8_t[m_numLeds];

  for(int idx = 0; idx<m_numLeds; idx++){
    m_onTime[idx] = pdMS_TO_TICKS(LED_DEFAULT_PULSE_LENGTH);
    m_offTime[idx] = pdMS_TO_TICKS(LED_DEFAULT_PULSE_LENGTH);
    m_brightSet[idx] = LED_DEFAULT_BRIGHT_SETTING;
    m_dimSet[idx] = LED_DEFAULT_DIM_SETTING;
    m_off[idx] = (m_led[idx].type == LedType_t::led_type_onoff) ? (m_led[idx].inverted ? HIGH : LOW) : (m_led[idx].inverted ? 255U : 0U);
    m_on[idx] = (m_led[idx].type == LedType_t::led_type_onoff) ? (m_led[idx].inverted ? LOW : HIGH) : (m_led[idx].inverted ? 0U : 255U);
    m_mode[idx] = LedMode_t::led_mode_off;
    m_state[idx] = LedState_t::led_state_off;
    m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_offTime[idx];
    if (m_led[idx].type == led_type_pwm) {
      m_channel[idx] = channel;
      channel++;
    } else {
      m_channel[idx] = LED_ONOFF_PIN_CHANNEL;
    }
  }
  m_timerHandle = xTimerCreate(t_name, pdMS_TO_TICKS(LED_UPDATE_INTERVAL), pdTRUE, this, ledTimerCallback);

  if (!m_timerHandle){log_e("Timer creation failed");}
}

DMCLedController::~DMCLedController(void){
  if (m_channel) delete[] m_channel;
  if (m_brightSet) delete[] m_brightSet;
  if (m_dimSet) delete[] m_dimSet;
  if (m_off) delete[] m_off;
  if (m_on) delete[] m_on;
  if (m_onTime) delete[] m_onTime;
  if (m_offTime) delete[] m_offTime;
  if (m_nextChangeTickCount) delete[] m_nextChangeTickCount;
  if (m_mode) delete[] m_mode;
  if (m_state) delete[] m_state;
}

BaseType_t DMCLedController::begin(void){
  if (m_err){
    log_d("LED Error: %d", m_err);
    return pdFAIL;
  }

  for(int idx = 0; idx<m_numLeds; idx++){
    if (m_led[idx].type == led_type_pwm) {
      ledcSetup(m_channel[idx], 5000U, LED_PWM_RESOLUTION);
      ledcAttachPin(m_led[idx].pin, m_channel[idx]);
    } else {
      pinMode(m_led[idx].pin, OUTPUT);
    }

    switch (m_state[idx]){
      case LedState_t::led_state_bright:
        ledcWrite(m_channel[idx], m_brightSet[idx]);
        m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_onTime[idx];
        break;
      
      case LedState_t::led_state_dim:
        ledcWrite(m_channel[idx], m_dimSet[idx]);
        if (m_mode[idx] == led_mode_dim_blink) {m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_onTime[idx];}
        else {m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_offTime[idx];}
        break;

      case LedState_t::led_state_off:
        if (m_led[idx].type == LedType_t::led_type_pwm){
          ledcWrite(m_channel[idx], m_off[idx]);
        } else {
          digitalWrite(m_led[idx].pin, m_off[idx]);
        }
        break;

      case LedState_t::led_state_on:
        if (m_led[idx].type == LedType_t::led_type_pwm){
          ledcWrite(m_channel[idx], m_on[idx]);
        } else {
          digitalWrite(m_led[idx].pin, m_on[idx]);
        }
        break;

      default:
    }

  }
  return xTimerStart(m_timerHandle, pdMS_TO_TICKS(TIMER_TIMEOUT));
}

TimerHandle_t DMCLedController::getTimerHandle(void){
  return m_timerHandle;
}

LedError_t DMCLedController::setBright(int t_id, int t_brightSet) {
  if (m_err){
    log_d("LED Error: %d", m_err);
    return m_err;
  }

  uint8_t cleanSetting = (t_brightSet < 0) ? 0 : ((t_brightSet > 255) ? 255 : t_brightSet);
  m_brightSet[t_id] = (m_led[t_id].inverted) ? 255 - t_brightSet : t_brightSet;
  if (m_state[t_id] = LedState_t::led_state_bright){
    ledcWrite(m_channel[t_id], m_brightSet[t_id]);
  }
  return LedError_t::led_OK;
}

LedError_t DMCLedController::setDim(int t_id, int t_dimSet) {
  if (m_err){
    log_d("LED Error: %d", m_err);
    return m_err;
  }

  uint8_t cleanSetting = (t_dimSet < 0) ? 0 : ((t_dimSet > 255) ? 255 : t_dimSet);
  m_dimSet[t_id] = (m_led[t_id].inverted) ? 255 - t_dimSet : t_dimSet;
  if (m_state[t_id] = LedState_t::led_state_dim){
    ledcWrite(m_channel[t_id], m_dimSet[t_id]);
  }
  return LedError_t::led_OK;
}

LedError_t DMCLedController::setMode(int t_id, LedMode_t t_mode){
  if (m_err){
    log_d("LED Error: %d", m_err);
    return m_err;
  }

  LedMode_t cleanMode = t_mode;

  if (m_led[t_id].type = LedType_t::led_type_onoff){
    switch (t_mode){
      case LedMode_t::led_mode_bright:
        cleanMode = LedMode_t::led_mode_on;
        break;

      case LedMode_t::led_mode_dim:
        cleanMode = LedMode_t::led_mode_off;
        break;

      case LedMode_t::led_mode_bright_blink:
        cleanMode = LedMode_t::led_mode_blink;
        break;

      case LedMode_t::led_mode_dim_blink:
        cleanMode = LedMode_t::led_mode_blink;
        break;

      case LedMode_t::led_mode_pulse:
        cleanMode = LedMode_t::led_mode_blink;
        break;
    }
  }

  switch (cleanMode){
    case LedMode_t::led_mode_off:
      if (m_led[t_id].type == LedType_t::led_type_pwm){
        ledcWrite(m_channel[t_id], m_off[t_id]);
      } else {
        digitalWrite(m_led[t_id].pin, m_off[t_id]);
      }
      m_state[t_id] = LedState_t::led_state_off;
      m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_offTime[t_id];
      break;

    case LedMode_t::led_mode_on:
      if (m_led[t_id].type == LedType_t::led_type_pwm){
        ledcWrite(m_channel[t_id], m_on[t_id]);
      } else {
        digitalWrite(m_led[t_id].pin, m_on[t_id]);
      }
      m_state[t_id] = LedState_t::led_state_off;
      m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_onTime[t_id];
      break;

    case LedMode_t::led_mode_dim:
      ledcWrite(m_channel[t_id], m_dimSet[t_id]);
      m_state[t_id] = LedState_t::led_state_dim;
      m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_offTime[t_id];
      break;

    case LedMode_t::led_mode_bright:
      ledcWrite(m_channel[t_id], m_brightSet[t_id]);
      m_state[t_id] = LedState_t::led_state_dim;
      break;

    case LedMode_t::led_mode_blink:
      if ((m_state[t_id] == LedState_t::led_state_dim) || (m_state[t_id] == LedState_t::led_state_off)) {
        if (m_led[t_id].type == LedType_t::led_type_onoff){
          digitalWrite(m_led[t_id].pin, m_on[t_id]);
        } else {
          ledcWrite(m_channel[t_id], m_on[t_id]);
        }
        m_state[t_id] == LedState_t::led_state_on;
        m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_onTime[t_id];
      } else {
        if (m_led[t_id].type == LedType_t::led_type_onoff){
          digitalWrite(m_led[t_id].pin, m_off[t_id]);
        } else {
          ledcWrite(m_channel[t_id], m_off[t_id]);
        }
        m_state[t_id] == LedState_t::led_state_off;
        m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_offTime[t_id];
      }
      break;

    case LedMode_t::led_mode_bright_blink:
      if (m_state[t_id] == LedState_t::led_state_bright) {
        ledcWrite(m_channel[t_id], m_off[t_id]);
        m_state[t_id] == LedState_t::led_state_off;
        m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_offTime[t_id];
      } else {
        ledcWrite(m_channel[t_id], m_brightSet[t_id]);
        m_state[t_id] == LedState_t::led_state_bright;
        m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_onTime[t_id];
      }
      break;

    case LedMode_t::led_mode_dim_blink:
      if (m_state[t_id] == LedState_t::led_state_dim) {
        ledcWrite(m_channel[t_id], m_off[t_id]);
        m_state[t_id] == LedState_t::led_state_off;
        m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_offTime[t_id];
      } else {
        ledcWrite(m_channel[t_id], m_dimSet[t_id]);
        m_state[t_id] == LedState_t::led_state_dim;
        m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_onTime[t_id];
      }
      break;

    case LedMode_t::led_mode_pulse:
      if ((m_state[t_id] == LedState_t::led_state_dim) || (m_state[t_id] == LedState_t::led_state_off)) {
        ledcWrite(m_channel[t_id], m_dimSet[t_id]);
        m_state[t_id] == LedState_t::led_state_dim;
        m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_offTime[t_id];
      } else {
        ledcWrite(m_channel[t_id], m_brightSet[t_id]);
        m_state[t_id] == LedState_t::led_state_bright;
        m_nextChangeTickCount[t_id] = xTaskGetTickCount() + m_onTime[t_id];
      }
      break;

    default:
      log_d("Invalid mode command: %d", t_mode);
      m_err = LedError_t::led_invalid_mode_command;
  }
  m_mode[t_id] = cleanMode;

  return m_err;
}

LedError_t DMCLedController::setOnTime(int t_id, uint32_t t_onTime){
  if (m_err){
    log_d("LED Error: %d", m_err);
    return m_err;
  }

  m_onTime[t_id] = pdMS_TO_TICKS(t_onTime);

  return LedError_t::led_OK;
}

LedError_t DMCLedController::setOffTime(int t_id, uint32_t t_offTime){
  if (m_err){
    log_d("LED Error: %d", m_err);
    return m_err;
  }

  m_offTime[t_id] = pdMS_TO_TICKS(t_offTime);

  return LedError_t::led_OK;
}

LedError_t DMCLedController::update(void){
  if (m_err){
    log_d("LED Error: %d", m_err);
    return m_err;
  }

  for (int idx = 0; idx < m_numLeds; idx++){
    switch (m_mode[idx]){
      case LedMode_t::led_mode_blink:
        if (xTaskGetTickCount() > m_nextChangeTickCount[idx]){
          switch (m_state[idx]){
            case LedState_t::led_state_on:
              if (m_led[idx].type == LedType_t::led_type_pwm) {ledcWrite(m_channel[idx], m_off[idx]);}
              else {digitalWrite(m_led[idx].pin, m_off[idx]);}
              m_state[idx] = LedState_t::led_state_off;
              m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_offTime[idx];
              break;

            default:
              if (m_led[idx].type == LedType_t::led_type_pwm) {ledcWrite(m_channel[idx], m_on[idx]);}
              else {digitalWrite(m_led[idx].pin, m_on[idx]);}
              m_state[idx] = LedState_t::led_state_on;
              m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_onTime[idx];
          }
        }
        break;

      case LedMode_t::led_mode_bright_blink:
        if (xTaskGetTickCount() > m_nextChangeTickCount[idx]){
          switch (m_state[idx]){
            case LedState_t::led_state_bright:
              ledcWrite(m_channel[idx], m_off[idx]);
              m_state[idx] = LedState_t::led_state_off;
              m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_offTime[idx];
              break;

            default:
              ledcWrite(m_channel[idx], m_brightSet[idx]);
              m_state[idx] = LedState_t::led_state_bright;
              m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_onTime[idx];
          }
        }
        break;

      case LedMode_t::led_mode_dim_blink:
        if (xTaskGetTickCount() > m_nextChangeTickCount[idx]){
          switch (m_state[idx]){
            case LedState_t::led_state_dim:
              ledcWrite(m_channel[idx], m_off[idx]);
              m_state[idx] = LedState_t::led_state_off;
              m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_offTime[idx];
              break;

            default:
              ledcWrite(m_channel[idx], m_dimSet[idx]);
              m_state[idx] = LedState_t::led_state_dim;
              m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_onTime[idx];
          }
        }
        break;

      case LedMode_t::led_mode_pulse:
        if (xTaskGetTickCount() > m_nextChangeTickCount[idx]){
          switch (m_state[idx]){
            case LedState_t::led_state_bright:
              ledcWrite(m_channel[idx], m_dimSet[idx]);
              m_state[idx] = LedState_t::led_state_dim;
              m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_offTime[idx];
              break;

            default:
              ledcWrite(m_channel[idx], m_brightSet[idx]);
              m_state[idx] = LedState_t::led_state_bright;
              m_nextChangeTickCount[idx] = xTaskGetTickCount() + m_onTime[idx];
          }
        }
        break;

      default:
    }
  }
}

void ledTimerCallback (TimerHandle_t t_xTimer){
  DMCLedController *controller;
  LedError_t err;
  controller = (DMCLedController *) pvTimerGetTimerID(t_xTimer);
  err = controller->update();
  if (err) {log_d("Update Error: %d",err);}
}
