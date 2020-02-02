#include "ESP8266VESC.h"

// The C files must be included specially for C++
extern "C"
{
    #include "crc.h"
    #include "buffer.h"
}

#include "esp_log.h"

#define BUF_SIZE 1024
#define ESPVESC_TAG "ESPVESC"

ESP8266VESC::ESP8266VESC(uart_port_t uart_num) : _uart_num(uart_num)
{
    /* Configure parameters of an UART driver,
    * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    ESP_ERROR_CHECK(uart_param_config(_uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(_uart_num, 16, 17, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(_uart_num, BUF_SIZE * 2, BUF_SIZE * 2, 10, NULL, 0));
}

void ESP8266VESC::setDutyCycle(const float dutyValue)
{
    int32_t index = 0;

    // Length: 1 byte packet ID and 4 bytes for float (32 bit)
    uint8_t payload[5] = {0};

    payload[index++] = COMM_SET_DUTY;

    float dutyScale = 100000.0f;
    buffer_append_float32(payload, dutyValue, dutyScale, &index);

    _sendPacket(payload, sizeof(payload));
}

void ESP8266VESC::setCurrent(const float currentInAmpere)
{
    int32_t index = 0;

    // Length: 1 byte packet ID and 4 bytes for integer (32 bit)
    uint8_t payload[5] = {0};

    payload[index++] = COMM_SET_CURRENT;

    int32_t currentInMilliampere = (int32_t) (currentInAmpere * 1000);
    buffer_append_int32(payload, currentInMilliampere, &index);

    _sendPacket(payload, sizeof(payload));
}

void ESP8266VESC::setCurrentBrake(const float currentInAmpere)
{
    int32_t index = 0;

    // Length: 1 byte packet ID and 4 bytes for integer (32 bit)
    uint8_t payload[5] = {0};

    payload[index++] = COMM_SET_CURRENT_BRAKE;

    int32_t currentInMilliampere = (int32_t) (currentInAmpere * 1000);
    buffer_append_int32(payload, currentInMilliampere, &index);

    _sendPacket(payload, sizeof(payload));
}

void ESP8266VESC::setRPM(const int32_t rpmValue)
{
    int32_t index = 0;

    // Length: 1 byte packet ID and 4 bytes for integer (32 bit)
    uint8_t payload[5] = {0};

    payload[index++] = COMM_SET_RPM;

    buffer_append_int32(payload, rpmValue, &index);

    _sendPacket(payload, sizeof(payload));
}

void ESP8266VESC::releaseEngine()
{
    // Set no engine current means idling
    setCurrent(0.0f);
}

void ESP8266VESC::fullBreaking()
{
    // Set duty cycle to 0.0 means full breaking
    setDutyCycle(0.0f);
}

bool ESP8266VESC::getVESCValues(VESCValues &vescValues)
{
    bool wasSuccessful = false;

    // Length: 1 byte packet ID
    uint8_t payload[1] = {0};
    payload[0] = COMM_GET_VALUES;

    // Send packet and wait some time to let the VESC process the request
    _sendPacket(payload, sizeof(payload));
    vTaskDelay(100 / portTICK_PERIOD_MS);

    uint8_t receivedPayload[256] = {0};
    uint16_t packetPayloadLength = _receivePacket(receivedPayload);

    // The packet has a minimum length of 56 bytes
    ESP_LOGI(ESPVESC_TAG, "payload length = %d", packetPayloadLength);
    if (packetPayloadLength >= 56)
    {
        COMM_PACKET_ID receivedPacketID = (COMM_PACKET_ID) receivedPayload[0];
        int32_t index = 0;

        // Skip the packet ID (first byte)
        index++;

        if (receivedPacketID == COMM_GET_VALUES)
        {
            vescValues.temperatureMosfet1 = buffer_get_float16(receivedPayload, 10.0, &index);
            vescValues.temperaturePCB = buffer_get_float16(receivedPayload, 10.0, &index);
            vescValues.avgMotorCurrent = buffer_get_float32(receivedPayload, 100.0, &index);
            vescValues.avgInputCurrent = buffer_get_float32(receivedPayload, 100.0, &index);
            buffer_get_float32(receivedPayload, 100.0, &index); // id
            buffer_get_float32(receivedPayload, 100.0, &index); // iq
            vescValues.dutyCycleNow = buffer_get_float16(receivedPayload, 10.0, &index);
            vescValues.rpm = buffer_get_int32(receivedPayload, &index);
            vescValues.inputVoltage = buffer_get_float16(receivedPayload, 10.0, &index);
            vescValues.ampHours = buffer_get_float32(receivedPayload, 10.0, &index);
            vescValues.ampHoursCharged = buffer_get_float32(receivedPayload, 10.0, &index);
            vescValues.wattHours = buffer_get_float32(receivedPayload, 10000.0, &index);
            vescValues.wattHoursCharged = buffer_get_float32(receivedPayload, 10000.0, &index);
            vescValues.tachometer = buffer_get_int32(receivedPayload, &index);
            vescValues.tachometerAbs = buffer_get_int32(receivedPayload, &index);
            vescValues.faultCode = (mc_fault_code) receivedPayload[index++];

            wasSuccessful = true;
        }
        else
        {
            ESP_LOGI(ESPVESC_TAG, "The received packet is not a COMM_GET_VALUES packet!");
            wasSuccessful = false;
        }
    }
    else
    {
        ESP_LOGE(ESPVESC_TAG, "The received packet is too short!");
        wasSuccessful = false;
    }

    return wasSuccessful;
}

uint16_t ESP8266VESC::_receivePacket(uint8_t packetPayload[])
{
    bool packetWasReceived = false;

    uint8_t packet[PACKET_MAX_LENGTH] = {0};
    uint16_t index = 0;

    uint16_t packetLength = 0;
    uint16_t packetPayloadLength = 0;

    // Read bytes until available
    size_t len = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(_uart_num, &len));
    ESP_LOGI(ESPVESC_TAG, "get buffered data len %d", len);

    while (len >= 1)
    {
        int readed = uart_read_bytes(_uart_num, &packet[index], 1, 20 / portTICK_RATE_MS);

        // When the index reaches 2, three bytes are received:
        // packet[0]: start byte indicating packet length type
        // packet[1]: packet length (first part)
        // packet[2]: packet length (second part if packet lenth type is PACKET_LENGTH_IDENTIFICATION_BYTE_LONG)
        if (index == 2)
        {
            uint8_t packetLengthType = packet[0];

            switch(packetLengthType)
            {
                case PACKET_LENGTH_IDENTIFICATION_BYTE_SHORT:
                {
                    packetPayloadLength = packet[1];

                    // Start byte + packet payload length byte + payload + 2 bytes (CRC) + termination byte
                    packetLength = 1 + 1 + packetPayloadLength + 2 + 1;
                }
                break;

                case PACKET_LENGTH_IDENTIFICATION_BYTE_LONG:
                {
                    // The length is splitted into 2 bytes
                    packetPayloadLength = (packet[1] << 8) | packet[2];

                    // Start byte + packet payload length bytes + payload + 2 bytes (CRC) + termination byte
                    packetLength = 1 + 2 + packetPayloadLength + 2 + 1;
                }
                break;
            }
        }

        // Give up if the packet is longer than the maximum packet length
        if (index >= PACKET_MAX_LENGTH)
        {
            ESP_LOGE(ESPVESC_TAG, "The received packet is too long!");
            break;
        }

        uint8_t lastPacketByte = packet[packetLength - 1];

        // When the index reaches the last byte (index begins at 0, so length - 1), check if the termination byte is set correctly
        if (index == packetLength - 1 && lastPacketByte == PACKET_TERMINATION_BYTE)
        {
            packetWasReceived = true;
            break;
        }

        // If still not broke out of the loop, increment the byte index
        index++;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(_uart_num, &len));
    }

    ESP_LOGI(ESPVESC_TAG, "received, len: %d", packetLength);

    uint16_t packetPayloadLengthIfPacketWasReceivedCorrectly = 0;

    if (packetWasReceived == true)
    {
        bool packetPayloadWasUnpacked = _unpackPacket(packet, packetPayload, packetLength, packetPayloadLength);

        if (packetPayloadWasUnpacked == true)
        {
            packetPayloadLengthIfPacketWasReceivedCorrectly = packetPayloadLength;
        }
    }

    return packetPayloadLengthIfPacketWasReceivedCorrectly;
}

bool ESP8266VESC::_unpackPacket(uint8_t packet[], uint8_t packetPayload[], uint16_t packetLength, uint16_t packetPayloadLength)
{
    bool packetPayloadWasUnpacked = false;

    uint8_t packetLengthType = packet[0];
    uint8_t packetPayloadStartIndex = 0;

    // Depends on the packet length type, the payload is starting on different index
    switch(packetLengthType)
    {
        case PACKET_LENGTH_IDENTIFICATION_BYTE_SHORT:
        {
            packetPayloadStartIndex = 2;
        }
        break;

        case PACKET_LENGTH_IDENTIFICATION_BYTE_LONG:
        {
            packetPayloadStartIndex = 3;
        }
        break;
    }

    // Copy payload from packet (start at payload start index)
    for (int i = 0; i < packetPayloadLength; i++) {
        packetPayload[i] = packet[packetPayloadStartIndex + i];
    }

    uint8_t firstCRCByte = packet[packetLength - 3];
    uint8_t secondCRCByte = packet[packetLength - 2];

    uint16_t receivedPacketCRC = (firstCRCByte << 8) | secondCRCByte;
    uint16_t calculatedPacketCRC = crc16(packetPayload, packetPayloadLength);

    // Check if packet was received correctly (CRC correct)
    if (receivedPacketCRC == calculatedPacketCRC)
    {
        packetPayloadWasUnpacked = true;
    }
    else
    {
        ESP_LOGE(ESPVESC_TAG, "CRC failed (expected: %d, got: %d)", calculatedPacketCRC, receivedPacketCRC);
    }

    return packetPayloadWasUnpacked;
}

void ESP8266VESC::_sendPacket(uint8_t packetPayload[], uint16_t packetPayloadLength)
{
    // Check if we got valid pointer and length is valid
    if ( !packetPayload || packetPayloadLength == 0 )
    {
        return ;
    }

    if ( packetPayloadLength > PACKET_PAYLOAD_MAX_LENGTH )
    {
        ESP_LOGE(ESPVESC_TAG, "The packet payload is larger than PACKET_PAYLOAD_MAX_LENGTH bytes!");
        return ;
    }

    uint8_t packet[PACKET_MAX_LENGTH] = {0};
    uint16_t index = 0;

    if (packetPayloadLength <= 256)
    {
        packet[index++] = PACKET_LENGTH_IDENTIFICATION_BYTE_SHORT;
        packet[index++] = packetPayloadLength;
    }
    else
    {
        packet[index++] = PACKET_LENGTH_IDENTIFICATION_BYTE_LONG;

        // The packet payload length is splitted up to 2 bytes
        packet[index++] = (uint8_t) (packetPayloadLength >> 8);
        packet[index++] = (uint8_t) (packetPayloadLength >> 0 & 0xFF);
    }

    // Copy payload to packet starting at index
    for (int i = 0; i < packetPayloadLength; i++) {
        packet[index + i] = packetPayload[i];
    }

    // Increment packet index by number of copied payload bytes
    index += packetPayloadLength;

    // CRC16 checksum (2 bytes)
    uint16_t crc16Checksum = crc16(packetPayload, packetPayloadLength);
    packet[index++] = (uint8_t) (crc16Checksum >> 8);
    packet[index++] = (uint8_t) (crc16Checksum >> 0 & 0xFF);

    // Termination byte
    packet[index++] = PACKET_TERMINATION_BYTE;

    // Write packet until index
    ESP_LOGI(ESPVESC_TAG, "write to serial %d len %d", _uart_num, index);
    int written = uart_write_bytes(_uart_num, (char *)packet, index);
    ESP_LOGI(ESPVESC_TAG, "bytes written: %d", written);
    ESP_ERROR_CHECK(uart_wait_tx_done(_uart_num, 100 / portTICK_PERIOD_MS));
}

void ESP8266VESC::debugPrintArray(uint8_t values[], int length)
{
    // Pointer sanity check
    if ( !values || length == 0 ) {
        return ;
    }

    for (int i = 0; i < length; i++)
    {
        ESP_LOGI(ESPVESC_TAG, "%02X ", values[i]);
    }
}
