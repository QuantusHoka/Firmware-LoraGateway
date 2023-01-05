// add heartbeat signal on all lora devices except gateway
// MQTT Structure:
//  messages to gateway for distribution are commands (for now): yurt/lora/cmnd/*DEVICE*/{param: data}json
//  messages from gateway for logging / node-red / monitoring are telemetry: yurt/lora/tele/*DEVICE*/data{param: data}json
//  mqtt status messages published on yurt/lora/tele/*DEVICE*/state

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <AsyncMQTT_ESP32.h>
#include <LoraGateway.h>
#include <DMC_LoRa.h>
#include <DMC_LED.h>
#include <DMC_Button.h>

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

QueueHandle_t SystemEventQueue;


//LED Setup
DMC_LED RedLed("RedLed", RED_LED_PIN, RED_LED_CHANNEL);
DMC_LED BlueLed("BlueLed", BLUE_LED_PIN, BLUE_LED_CHANNEL);
DMC_LED GreenLed("GreenLed", GREEN_LED_PIN, GREEN_LED_CHANNEL);

//Button Setup
static DMC_Button Button("Button",BUTTON_PIN, &SystemEventQueue, sys_event_ButtonLongPress, sys_event_ButtonSingleClick, sys_event_ButtonDoubleClick);

// System event handler
void task_systemEventHandler(void *params);
//void systemDispatch(SystemEvent_t system_event);

void task_LoRaMessageProcessor(void *params);

//Connectivity
void connectToWifi(void);
void connectToMqtt(void);
void WiFiEvent(WiFiEvent_t event);
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe(const uint16_t& packetId, const uint8_t& qos);
void onMqttUnsubscribe(const uint16_t& packetId);
void onMqttPublish(const uint16_t& packetId);
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

//Other Functions
void setupLED(DMC_LED* LED);

void setup() {
  bool SetupErr = false;

  Serial.begin(19200);
  Serial.setDebugOutput(true);
  delay(500);

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  
  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASS);

  connectToWifi();

  log_v("WiFi Set-up");

  SystemEventQueue = xQueueCreate(20U, sizeof(SystemEvent_t));

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, NSS_PIN);
  DMCLoRa.setSPI(SPI);
  DMCLoRa.setPins(NSS_PIN, RST_PIN, DIO0_PIN);
  DMCLoRa.disableCrc();
  DMCLoRa.begin(LORA_FREQUENCY, LORA_YURT_ADDRESS, &SystemEventQueue, sys_event_MsgReceived, sys_event_MsgSendFail, sys_event_LoRaFailed);
  Button.begin();
  RedLed.begin();
  setupLED(&RedLed);
  GreenLed.begin();
  setupLED(&GreenLed);
  LedEvent_t LEDSetting = {Led_state_dimpulse, 0U};
  xQueueSendToBack(GreenLed.EventQueue, &LEDSetting, portMAX_DELAY);
  BlueLed.begin();
  setupLED(&BlueLed);

  xTaskCreate(task_systemEventHandler, "SystemEventHandler", SYSTEM_EVENT_HANDLER_TASK_STACK_DEPTH, NULL, SYSTEM_EVENT_HANDLER_TASK_PRIORITY, NULL);
  xTaskCreate(task_LoRaMessageProcessor, "LoRaMessageProcessor", LORA_MESSAGE_PROCESSOR_TASK_STACK_DEPTH, NULL, LORA_MESSAGE_PROCESSOR_TASK_PRIORITY, NULL);

  log_v("Buttons set-up");

  if(!SetupErr) {
    LEDSetting.code = Led_state_dim; LEDSetting.value = 0;
    xQueueSendToBack(GreenLed.EventQueue, &LEDSetting, portMAX_DELAY);
  }
  else {
    LEDSetting.code = Led_state_off; LEDSetting.value = 0;
    xQueueSendToBack(GreenLed.EventQueue, &LEDSetting, portMAX_DELAY);
    LEDSetting.code = Led_state_dimpulse; LEDSetting.value = 0;
    xQueueSendToBack(RedLed.EventQueue, &LEDSetting, portMAX_DELAY);
  }
}

void loop() {
  DMCLoRa.sendMessage(LORA_POWER_TRAILER_ADDRESS, "switch");
  vTaskDelay(pdMS_TO_TICKS(5000));
}

void connectToWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void connectToMqtt() {
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      log_w("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
    }
}

void onMqttConnect(bool sessionPresent) {
  log_d("Connected to MQTT.");
  uint16_t packetIdSub = mqttClient.subscribe("yurt/lora/cmnd/#", 1);
  log_v("Subscribing yurt/lora/cmnd/# at QoS 2");
  mqttClient.publish("yurt/lora/tele/gateway/state", 0, false, "online");
  log_v("Publishing at QoS 0");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  log_w("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(const uint16_t& packetId, const uint8_t& qos)
{
  log_v("Subscribe acknowledged.");
  log_v("  packetId: %u,  qos: %u", packetId, qos);
}

void onMqttUnsubscribe(const uint16_t& packetId)
{
  log_v("Unsubscribe acknowledged.");
  log_v("  packetId: %u", packetId);
}

void onMqttPublish(const uint16_t& packetId)
{
  log_v("Publish acknowledged.");
  log_v("  packetId: %u", packetId);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  log_v("MQTT message received.\n");
  log_v("Topic: %s  len: %u  index: %u", topic, len, index);
  log_v("%s", payload);
}

void setupLED(DMC_LED* LED){
  LedEvent_t setting = {Led_set_dim, LED_DIM};
  xQueueSendToBack(LED->EventQueue, &setting, portMAX_DELAY);
  setting.code = Led_set_bright; setting.value = LED_BRIGHT;
  xQueueSendToBack(LED->EventQueue, &setting, portMAX_DELAY);
  setting.code = Led_state_off; setting.value = 0U;
}

void task_LoRaMessageProcessor(void *params){
  DMC_LoRaMessage_t message;
  String MQTTTopic;

  while(1){
    xQueueReceive(DMCLoRa.RxQueue, &message, portMAX_DELAY);
    if (message.From == LORA_POWER_TRAILER_ADDRESS){
      mqttClient.publish("yurt/lora/tele/power/DATA", 0, false, message.payload);
    }
    else{
      mqttClient.publish("yurt/lora/tele/gateway/INCOMING", 0, false, message.payload);
    }
  }
  vTaskDelete(NULL);
}

void task_systemEventHandler(void *params){
  SystemEvent_t  systemevent;

  while(1){
    xQueueReceive(SystemEventQueue, &systemevent, portMAX_DELAY);
    log_v("Received system event: %u", systemevent);
    switch (systemevent)
    {
    case sys_event_ButtonSingleClick:
        DMCLoRa.sendMessage(LORA_POWER_TRAILER_ADDRESS, "switch");
        mqttClient.publish("yurt/lora/tele/gateway/TESTING", 0, false, "Button");
      break;
    
    default:
      break;
    }
  }
  vTaskDelete(NULL);
}