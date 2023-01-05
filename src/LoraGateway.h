#ifndef LORA_GATEWAY_H
#define LORA_GATEWAY_H

#include <DMC_LoRa.h>

//LORA Addresses
enum LoRaAddresses {LORA_YURT_ADDRESS, LORA_POWER_TRAILER_ADDRESS, LORA_BROADCAST_ADDRESS = LORA_BROADCAST_ID};

//Pin Assignments
//Outputs
#define RED_LED_PIN D7 //high = on; will use pwm
#define BLUE_LED_PIN D2 //high = on; will use pwm
#define GREEN_LED_PIN D3 //high = on; will use pwm

//SPI
#define SCK_PIN SCK
#define MISO_PIN MISO
#define MOSI_PIN MOSI

//RFM
#define DIO0_PIN D9 //RFM95 DIO0
#define RST_PIN D6 //RFM95 RST
#define NSS_PIN GPIO_NUM_4 //RFM95 NSS

//INPUTS
#define BUTTON_PIN A4 //High = pressed


//System Settings
#define SYSTEM_EVENT_HANDLER_TASK_STACK_DEPTH 0x900
#define SYSTEM_EVENT_HANDLER_TASK_PRIORITY 5U
#define LORA_MESSAGE_PROCESSOR_TASK_STACK_DEPTH 0x900
#define LORA_MESSAGE_PROCESSOR_TASK_PRIORITY 5U
#define WIFI_SSID "Yurt"
#define WIFI_PASS "Vsd5h#5e9"
#define MQTT_SERVER "hollowhillsiot.duckdns.org"
#define MQTT_PORT 1883
#define MQTT_USER "hhiot"
#define MQTT_PASS "2k5sC4rft"
#define LED_DIM 128U
#define LED_BRIGHT 255U
#define RED_LED_CHANNEL 0U
#define GREEN_LED_CHANNEL 1U
#define BLUE_LED_CHANNEL 2U
#define LORA_FREQUENCY 915E6 // LoRa Frequency

//System Events
typedef enum {
    sys_event_Init,
    sys_event_ButtonLongPress,
    sys_event_ButtonSingleClick,
    sys_event_ButtonDoubleClick,
    sys_event_OtherGenStart,
    sys_event_OtherGenStop,
    sys_event_AlertOn,
    sys_event_AlertOff,
    sys_event_MsgSendFail,
    sys_event_MsgReceived,
    sys_event_LoRaFailed,
    sys_event_Restart,
    sys_event_UnknownError = 0xff
} SystemEvent_t;

#endif