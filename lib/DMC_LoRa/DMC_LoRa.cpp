#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <DMC_LoRa.h>


DMC_LoRa_Class::DMC_LoRa_Class()
{
    MyID = 0xff;
    _msgID = 0;
    MsgTxFailCode = 0xff;
    MsgRxCode = 0xff;
    LoraFailCode = 0xff;

    RxQueue = xQueueCreate(10U, sizeof(DMC_LoRaMessage_t));
    AckQueue = xQueueCreate(10U, sizeof(DMC_Ack_t));
    TxQueue = xQueueCreate(10U, sizeof(DMC_LoRaMessage_t));
}

void DMC_LoRa_Class::begin(long frequency, byte myid, QueueHandle_t *system_event_queue, int message_rx_code, int message_tx_fail_code, int lora_fail_code)
{
    MyID = myid;
    SystemEventQueue = system_event_queue;
    MsgRxCode = message_rx_code;
    MsgTxFailCode = message_tx_fail_code;
    LoraFailCode = lora_fail_code;

    if (!LoRaClass::begin(frequency)){
        log_w("LoRa failed to start properly");
        xQueueSendToBack(*SystemEventQueue, &LoraFailCode, portMAX_DELAY);
    }
    else {
        xTaskCreatePinnedToCore(task_DMC_LoRa, "DMC_LoRa_Processor", LORA_PROCESSOR_STACK_DEPTH, NULL, 10U, NULL, 1);
        log_v("LoRa Task Started");
    }
}

bool DMC_LoRa_Class::sendMessage(byte to, const char* payload, int id)
{
    bool err = true;
    if (!(sizeof(payload) > LORA_MAX_PAYLOAD_LEN)){
        DMC_LoRaMessage_t packet;
        packet.To = to;
        packet.From = MyID;
        if (id >= 0) {
            _msgID = (_msgID == (byte)0xff) ? (byte)0 : _msgID + (byte)0x01;
            packet.ID = _msgID;
        } else { packet.ID = _msgID;}
        packet.Length = sizeof(payload);
        strlcpy(packet.payload, payload, packet.Length);
        xQueueSendToBack(TxQueue, &packet, pdMS_TO_TICKS(5000));
        err = true;
    } else
    {
        log_d("message too big");
        xQueueSendToBack(*SystemEventQueue, &MsgTxFailCode, portMAX_DELAY);
        err = false;
    }
    return err;
}

void DMC_LoRa_onreceive(int packetsize)
{
    log_v("LoRa message received...");
    if (packetsize){
        DMC_LoRaMessage_t packet;
        packet.To = DMCLoRa.read();
        packet.From = DMCLoRa.read();
        packet.ID = DMCLoRa.read();
        packet.Length = DMCLoRa.read();
        int i = 0;
        while(DMCLoRa.available()){
            packet.payload[i] = (char)DMCLoRa.read();
            i++;
        }
        log_v("To: %u, From: %u, ID: %u, Length: %u, Size: %i", packet.To, packet.From, packet.ID, packet.Length, packetsize);
        log_v("Payload: %s", packet.payload);

        // Check if message is for us
        if (packet.To == DMCLoRa.MyID){ //Sent directly to me
            // Check if message is ACK
            if (strcmp(packet.payload, LORA_ACK_MESSAGE)){ //is an ACK packet
                DMC_Ack_t ackPacket;
                ackPacket.From = packet.From;
                ackPacket.ID = packet.ID;
                xQueueSendToBack(DMCLoRa.AckQueue, &ackPacket, 0U);
            }
            else{ //is not an ACK packet (and is sent directly to me)
                xQueueSendToBack(DMCLoRa.RxQueue, &packet, pdMS_TO_TICKS(500));
                DMCLoRa.sendMessage(packet.From, LORA_ACK_MESSAGE);
            }
        } else if (packet.To == LORA_BROADCAST_ID) { //Is a broadcast message
            xQueueSendToBack(DMCLoRa.RxQueue, &packet, pdMS_TO_TICKS(500));
        }
    }
    else {log_v("Packet length 0");}
}

void task_DMC_LoRa(void *params)
{
    int SystemEventCode;

//Start LoRa
    log_v("Starting LoRa Task");

    DMCLoRa.onReceive(DMC_LoRa_onreceive);
//    DMCLoRa.enableCrc();
    DMCLoRa.receive();

    log_v("LoRa started");

    DMC_LoRaMessage_t packet;
    int SendRetryCount = 0;
    DMC_Ack_t AckPacket;
    bool ACKReceived = false;

    while(1){
        xQueueReceive(DMCLoRa.TxQueue, &packet, portMAX_DELAY); //wait for a message to added to the send queue
        log_v("message to send");
        SendRetryCount = 0;
        ACKReceived = false;
        while ((SendRetryCount < LORA_SEND_MAX_RETRIES) && !ACKReceived){
            DMCLoRa.beginPacket();
            DMCLoRa.write(packet.To);
            DMCLoRa.write(packet.From);
            DMCLoRa.write(packet.ID);
            DMCLoRa.write(packet.Length);
            DMCLoRa.print(packet.payload);
            DMCLoRa.endPacket();
            log_v("Packet sent");
            DMCLoRa.receive();
            if (xQueueReceive(DMCLoRa.AckQueue, &AckPacket, pdMS_TO_TICKS(LORA_SEND_RETRY_TIMEOUT))) { //wait for ACK or timout
                if ((AckPacket.From == packet.To) && (AckPacket.ID == packet.ID)){
                    ACKReceived = true;
                    log_v("ACK received");
                }
            }
            else {
                SendRetryCount++;
                log_v("Waiting for ACK. Retry: %u", SendRetryCount);
            }
        }
        if (!ACKReceived){
            log_v("No ACK. Send failed");
            xQueueSendToBack(*DMCLoRa.SystemEventQueue, &DMCLoRa.MsgTxFailCode, pdMS_TO_TICKS(500));
        }
    }
    vTaskDelete(NULL);
}

DMC_LoRa_Class DMCLoRa;