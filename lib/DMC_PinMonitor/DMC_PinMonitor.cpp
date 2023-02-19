#include <Arduino.h>
#include <DMC_PinMonitor.h>


DMCPinMonitor::DMCPinMonitor(const char *t_name, DMC_Pin *t_pins){
    m_numPins = sizeof(t_pins) / sizeof(DMC_Pin);
    m_pin = t_pins;
    m_hx = new uint16_t[m_numPins];
    m_timerHandle = xTimerCreate(t_name, pdMS_TO_TICKS(PIN_MONITOR_INTERVAL), pdTRUE, this, pinTimerCallback);
}

DMCPinMonitor::~DMCPinMonitor(void){
    if (m_hx) delete[] m_hx;
}

BaseType_t DMCPinMonitor::begin(void){
    for (int idx = 0; idx < m_numPins; idx++){
        pinMode(m_pin[idx].pinNumber, m_pin[idx].mode);
        m_hx[idx] = digitalRead(m_pin[idx].pinNumber) ? PIN_HIGH_SET : PIN_LOW_SET;
        log_v("Pin %d at init: %d", idx, m_hx[idx]);
    }
    return xTimerStart(m_timerHandle, pdMS_TO_TICKS(TIMER_TIMEOUT));
}

bool DMCPinMonitor::update(void){
    int msg = 0;
    for(int idx = 0; idx < m_numPins; idx++){
        m_hx[idx] = (m_hx[idx] << 1);
        m_hx[idx] |= digitalRead(m_pin[idx].pinNumber);
        if ((m_hx[idx] & PIN_HX_MASK) == PIN_HIGH) {m_hx[idx] = PIN_HIGH_SET; msg = m_pin[idx].highMessage; xQueueSendToBack(*m_pin[idx].eventQueue, &msg, 0U);}
        if ((m_hx[idx] & PIN_HX_MASK) == PIN_LOW) {m_hx[idx] = PIN_LOW_SET; msg = m_pin[idx].lowMessage; xQueueSendToBack(*m_pin[idx].eventQueue, &msg, 0U);}
    }
    return true;
}

void pinTimerCallback(TimerHandle_t t_xtimer){
    DMCPinMonitor *monitor;
    bool err;
    monitor = (DMCPinMonitor *) pvTimerGetTimerID(t_xtimer);
    err = monitor->update();
    if (err) {log_d("Pinmonitor callback error");}
}