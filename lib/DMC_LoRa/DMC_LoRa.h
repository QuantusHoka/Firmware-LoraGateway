// A library for RTOS implementation of 2-way LoRa
// Will use implicit header for DMC_LoRa
// LoRa message Structure: 
// Preamble (not counted in PKT) - To(byte) + From (byte) + ID (byte) + PayloadLen (byte) + Payload (variable) + CRC (2 x byte - automatically stripped on Rx)

//This library will use the default SPI bus for now. Be sure to call SPI.start(SCK, MOSI, MISO, CS) before DMCLoRa.start() in main
//if using non-standard pins for SPI.

//Messages are sent using DMCLoRa.sendMessage(byte to, char* payload)
//Messages are received by 1. indicating that a message is available in the system event queue (defined in main), and then reading from
//DMCLoRa_rxQueue.
//Messages in rxQueue are of type DMC_LoRaMsg_t

//NOTE: address / ID 0xff is ALWAYS "Broadcast"

#ifndef DMC_LORA_H
#define DMC_LORA_H

#include <Arduino.h>
#include <LoRa.h>

#define LORA_MAX_PKT_LEN 255U
#define LORA_EXPLICIT_HEADER_LEN 4U
#define LORA_MAX_PAYLOAD_LEN LORA_MAX_PKT_LEN + LORA_EXPLICIT_HEADER_LEN - 6U //bytes

#define LORA_PROCESSOR_STACK_DEPTH 0x800

#define LORA_SEND_ACK_TIMEOUT 2000 //ms
#define LORA_SEND_MAX_RETRIES 5

#define LORA_BROADCAST_ID 0xff
#define LORA_ACK_MESSAGE "ACKACK"
#define LORA_MAX_SEND_RETRIES 5
#define LORA_SEND_RETRY_TIMEOUT 2000 //ms

struct DMC_LoRaMessage_t
{
    byte To, From, ID, Length;
    char payload[LORA_MAX_PAYLOAD_LEN];
};

struct DMC_Ack_t
{
    byte From, ID;
};

class DMC_LoRa_Class : public LoRaClass
{
    public:
    byte MyID;
    int MsgTxFailCode;
    int MsgRxCode;
    int LoraFailCode;
    QueueHandle_t RxQueue;
    QueueHandle_t AckQueue;
    QueueHandle_t TxQueue;
    QueueHandle_t *SystemEventQueue;

    DMC_LoRa_Class();
    
    void begin(long frequency, byte myid, QueueHandle_t *system_event_queue, int message_rx_code, int message_tx_fail_code = 0xff, int lora_fail_code = 0xff);
    bool sendMessage(byte to, const char* payload, int id = -1);

    private:
    uint8_t _msgID;
    int8_t _nss, _rst, _dio0;

};

void task_DMC_LoRa(void *params);

void DMC_LoRa_onreceive(int packetsize);

extern DMC_LoRa_Class DMCLoRa;

#endif


