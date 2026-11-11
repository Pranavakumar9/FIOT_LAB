/*
 * ESPLib.c
 * ESP8266 AT-command driver and small MQTT 3.1.1 packet builder.
 *
 * IMPORTANT: This file contains library functions only.
 * Do not put main(), huart1, SystemClock_Config(), or Error_Handler()
 * in this file. Those application definitions belong in main.c only.
 */

#include "ESPLib.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef *esp_uart = NULL;
static uint16_t responseLength = 0U;

char Buffer[BUFFERSIZE];
char rxBuffer[BUFFERSIZE];
char mqttBuffer[BUFFERSIZE];
char mqttPacket[BUFFERSIZE];

/* Original-library compatibility variables. */
uint8_t uartRxData = 0U;
uint16_t DataCounter = 0U;
volatile uint8_t RxInterruptFlag = 0U;
volatile uint8_t uartTimeCounter = 0U;
volatile uint8_t uartPacketComplatedFlag = 0U;
volatile uint8_t temp_mqttMsgLen = 0U;
volatile uint8_t temp_mqttTopicLen = 0U;

static bool buffer_contains(const char *needle)
{
    size_t needleLength;
    uint16_t i;

    if ((needle == NULL) || (needle[0] == '\0'))
    {
        return false;
    }

    needleLength = strlen(needle);
    if (needleLength > responseLength)
    {
        return false;
    }

    for (i = 0U; i <= (uint16_t)(responseLength - needleLength); i++)
    {
        if (memcmp(&Buffer[i], needle, needleLength) == 0)
        {
            return true;
        }
    }

    return false;
}

static void clear_response(void)
{
    memset(Buffer, 0, sizeof(Buffer));
    responseLength = 0U;
}

/* Read UART bytes until a required text token arrives or timeout expires.
 * The scan uses a length rather than strlen(), because MQTT data can contain
 * zero bytes while the ESP8266 is reporting received TCP data. */
static funcState_t wait_for_response(const char *success1,
                                     const char *success2,
                                     uint32_t timeoutMs)
{
    uint32_t startTime;
    uint8_t byteValue;

    clear_response();
    startTime = HAL_GetTick();

    while ((HAL_GetTick() - startTime) < timeoutMs)
    {
        HAL_StatusTypeDef status;

        byteValue = 0U;
        status = HAL_UART_Receive(esp_uart, &byteValue, 1U, 10U);

        if (status == HAL_OK)
        {
            if (responseLength >= (BUFFERSIZE - 1U))
            {
                memmove(Buffer, &Buffer[1], BUFFERSIZE - 2U);
                responseLength = BUFFERSIZE - 2U;
            }

            Buffer[responseLength++] = (char)byteValue;
            Buffer[responseLength] = '\0';

            if (buffer_contains(success1) || buffer_contains(success2))
            {
                return funcOk;
            }

            if (buffer_contains("ERROR") ||
                buffer_contains("FAIL") ||
                buffer_contains("busy p"))
            {
                return funcErr;
            }
        }
        else if ((status != HAL_TIMEOUT) && (status != HAL_BUSY))
        {
            return funcErr;
        }
    }

    return funcErr;
}

static funcState_t send_command(const char *command,
                                const char *success1,
                                const char *success2,
                                uint32_t timeoutMs)
{
    size_t commandLength;

    if ((esp_uart == NULL) || (command == NULL))
    {
        return funcErr;
    }

    commandLength = strlen(command);
    if (HAL_UART_Transmit(esp_uart,
                          (uint8_t *)command,
                          (uint16_t)commandLength,
                          2000U) != HAL_OK)
    {
        return funcErr;
    }

    return wait_for_response(success1, success2, timeoutMs);
}

funcState_t ESP8266_Init(espMode_t mode, UART_HandleTypeDef *uartHandle)
{
    char command[32];
    int written;
    uint8_t attempt;

    if (uartHandle == NULL)
    {
        return funcErr;
    }

    /* Keep the original HAL handle; do not make a second copy of its state. */
    esp_uart = uartHandle;

    /* Check that the ESP8266 responds to AT. */
    for (attempt = 0U; attempt < 5U; attempt++)
    {
        if (send_command("AT\r\n", "OK", NULL, 1000U) == funcOk)
        {
            break;
        }
        HAL_Delay(250U);
    }

    if (attempt >= 5U)
    {
        return funcErr;
    }

    /* Disable command echo. Some firmware versions may not support ATE0;
       failure here is non-fatal, so the next command is still attempted. */
    (void)send_command("ATE0\r\n", "OK", NULL, 1000U);

    written = snprintf(command, sizeof(command), "AT+CWMODE=%u\r\n", (unsigned int)mode);
    if ((written <= 0) || ((size_t)written >= sizeof(command)))
    {
        return funcErr;
    }

    return send_command(command, "OK", NULL, 3000U);
}

funcState_t ESP8266_Reset(void)
{
    funcState_t result;

    if (esp_uart == NULL)
    {
        return funcErr;
    }

    result = send_command("AT+RST\r\n", "ready", "OK", 8000U);
    HAL_Delay(1000U);
    return result;
}

funcState_t ESP8266_wifiConnect(char *SSID, char *Password)
{
    char command[160];
    int written;

    if ((SSID == NULL) || (Password == NULL))
    {
        return funcErr;
    }

    written = snprintf(command, sizeof(command),
                       "AT+CWJAP=\"%s\",\"%s\"\r\n",
                       SSID, Password);

    if ((written <= 0) || ((size_t)written >= sizeof(command)))
    {
        return funcErr;
    }

    /* ESP-AT normally reports WIFI GOT IP after a successful join. */
    return send_command(command, "WIFI GOT IP", "OK", 30000U);
}

funcState_t ESP8266_portConnect(char *type, char *remoteIP, char *remotePort)
{
    char command[160];
    int written;

    if ((type == NULL) || (remoteIP == NULL) || (remotePort == NULL))
    {
        return funcErr;
    }

    if (send_command("AT+CIPMUX=0\r\n", "OK", NULL, 3000U) != funcOk)
    {
        return funcErr;
    }

    written = snprintf(command, sizeof(command),
                       "AT+CIPSTART=\"%s\",\"%s\",%s\r\n",
                       type, remoteIP, remotePort);

    if ((written <= 0) || ((size_t)written >= sizeof(command)))
    {
        return funcErr;
    }

    return send_command(command, "CONNECT", "ALREADY CONNECTED", 20000U);
}

funcState_t ESP8266_sendMessage(char *msg, uint8_t msgSize)
{
    char command[32];
    int written;

    if ((esp_uart == NULL) || (msg == NULL) ||
        (msgSize == 0U) || (msgSize > BUFFERSIZE))
    {
        return funcErr;
    }

    written = snprintf(command, sizeof(command), "AT+CIPSEND=%u\r\n",
                       (unsigned int)msgSize);

    if ((written <= 0) || ((size_t)written >= sizeof(command)))
    {
        return funcErr;
    }

    if (HAL_UART_Transmit(esp_uart, (uint8_t *)command,
                          (uint16_t)written, 2000U) != HAL_OK)
    {
        return funcErr;
    }

    /* ESP8266 prompt indicates that it is ready for the TCP payload. */
    if (wait_for_response(">", NULL, 5000U) != funcOk)
    {
        return funcErr;
    }

    /* Send exactly msgSize bytes; MQTT packets are binary, not C strings. */
    if (HAL_UART_Transmit(esp_uart, (uint8_t *)msg,
                          msgSize, 5000U) != HAL_OK)
    {
        return funcErr;
    }

    return wait_for_response("SEND OK", NULL, 8000U);
}

funcState_t checkResponse(char *response)
{
    if (response == NULL)
    {
        return funcErr;
    }

    return buffer_contains(response) ? funcOk : funcErr;
}

static bool mqtt_prepare(uint8_t firstByte, uint16_t remainingLength,
                         uint16_t *totalLength)
{
    if ((totalLength == NULL) || (remainingLength > 127U) ||
        ((remainingLength + 2U) > BUFFERSIZE))
    {
        return false;
    }

    memset(mqttPacket, 0, sizeof(mqttPacket));
    mqttPacket[0] = (char)firstByte;
    mqttPacket[1] = (char)remainingLength;
    *totalLength = (uint16_t)(remainingLength + 2U);
    return true;
}

void MQTT_connectBroker(MQTT_InitTypeDef_t *MQTTConnect)
{
    uint8_t *packet = (uint8_t *)mqttPacket;
    uint16_t clientLength;
    uint16_t usernameLength = 0U;
    uint16_t passwordLength = 0U;
    uint16_t remainingLength;
    uint16_t totalLength;
    uint16_t position;
    uint8_t flags;

    if ((MQTTConnect == NULL) || (MQTTConnect->clientID == NULL))
    {
        return;
    }

    flags = MQTTConnect->connectFlag;
    clientLength = (uint16_t)strlen(MQTTConnect->clientID);

    if ((flags & UserNameFlag) != 0U)
    {
        if (MQTTConnect->username == NULL) return;
        usernameLength = (uint16_t)strlen(MQTTConnect->username);
    }

    if ((flags & PasswordFlag) != 0U)
    {
        if (MQTTConnect->password == NULL) return;
        passwordLength = (uint16_t)strlen(MQTTConnect->password);
    }

    remainingLength = (uint16_t)(12U + clientLength);
    if ((flags & UserNameFlag) != 0U) remainingLength += (uint16_t)(2U + usernameLength);
    if ((flags & PasswordFlag) != 0U) remainingLength += (uint16_t)(2U + passwordLength);

    if (!mqtt_prepare(MQTTconnect, remainingLength, &totalLength))
    {
        return;
    }

    /* MQTT 3.1.1 variable header. */
    packet[2] = 0x00U;
    packet[3] = 0x04U;
    packet[4] = 'M'; packet[5] = 'Q'; packet[6] = 'T'; packet[7] = 'T';
    packet[8] = MQTTlevel;
    packet[9] = flags;
    packet[10] = (uint8_t)(MQTTConnect->keepAlive >> 8);
    packet[11] = (uint8_t)(MQTTConnect->keepAlive & 0xFFU);

    /* Payload: Client ID. */
    position = 12U;
    packet[position++] = (uint8_t)(clientLength >> 8);
    packet[position++] = (uint8_t)(clientLength & 0xFFU);
    memcpy(&packet[position], MQTTConnect->clientID, clientLength);
    position = (uint16_t)(position + clientLength);

    if ((flags & UserNameFlag) != 0U)
    {
        packet[position++] = (uint8_t)(usernameLength >> 8);
        packet[position++] = (uint8_t)(usernameLength & 0xFFU);
        memcpy(&packet[position], MQTTConnect->username, usernameLength);
        position = (uint16_t)(position + usernameLength);
    }

    if ((flags & PasswordFlag) != 0U)
    {
        packet[position++] = (uint8_t)(passwordLength >> 8);
        packet[position++] = (uint8_t)(passwordLength & 0xFFU);
        memcpy(&packet[position], MQTTConnect->password, passwordLength);
    }

    (void)ESP8266_sendMessage(mqttPacket, (uint8_t)totalLength);
}

void MQTT_publishTopic(char *topic, char *msg, uint8_t QoS)
{
    uint8_t *packet = (uint8_t *)mqttPacket;
    uint16_t topicLength;
    uint16_t messageLength;
    uint16_t remainingLength;
    uint16_t totalLength;
    uint16_t position;

    /* This project publishes with QoS 0. QoS 1/2 need packet identifiers. */
    if ((topic == NULL) || (msg == NULL) || (QoS != QoS0))
    {
        return;
    }

    topicLength = (uint16_t)strlen(topic);
    messageLength = (uint16_t)strlen(msg);
    remainingLength = (uint16_t)(2U + topicLength + messageLength);

    if (!mqtt_prepare(MQTTpublish, remainingLength, &totalLength))
    {
        return;
    }

    packet[2] = (uint8_t)(topicLength >> 8);
    packet[3] = (uint8_t)(topicLength & 0xFFU);
    position = 4U;
    memcpy(&packet[position], topic, topicLength);
    position = (uint16_t)(position + topicLength);
    memcpy(&packet[position], msg, messageLength);

    (void)ESP8266_sendMessage(mqttPacket, (uint8_t)totalLength);
}

void MQTT_subscribeTopic(char *topic, uint8_t QoS)
{
    uint8_t *packet = (uint8_t *)mqttPacket;
    uint16_t topicLength;
    uint16_t remainingLength;
    uint16_t totalLength;
    uint8_t requestedQos;

    if (topic == NULL) return;
    if (QoS == QoS0) requestedQos = 0U;
    else if (QoS == QoS1) requestedQos = 1U;
    else if (QoS == QoS2) requestedQos = 2U;
    else return;

    topicLength = (uint16_t)strlen(topic);
    remainingLength = (uint16_t)(5U + topicLength);
    if (!mqtt_prepare(MQTTsubscribe, remainingLength, &totalLength)) return;

    (void)totalLength;
    packet[2] = 0x00U; packet[3] = 0x01U; /* Packet identifier */
    packet[4] = (uint8_t)(topicLength >> 8);
    packet[5] = (uint8_t)(topicLength & 0xFFU);
    memcpy(&packet[6], topic, topicLength);
    packet[6U + topicLength] = requestedQos;

    (void)ESP8266_sendMessage(mqttPacket, (uint8_t)(remainingLength + 2U));
}

void MQTT_unsubscribeTopic(char *topic)
{
    uint8_t *packet = (uint8_t *)mqttPacket;
    uint16_t topicLength;
    uint16_t remainingLength;
    uint16_t totalLength;

    if (topic == NULL) return;
    topicLength = (uint16_t)strlen(topic);
    remainingLength = (uint16_t)(4U + topicLength);
    if (!mqtt_prepare(MQTTunsub, remainingLength, &totalLength)) return;

    packet[2] = 0x00U; packet[3] = 0x01U;
    packet[4] = (uint8_t)(topicLength >> 8);
    packet[5] = (uint8_t)(topicLength & 0xFFU);
    memcpy(&packet[6], topic, topicLength);

    (void)ESP8266_sendMessage(mqttPacket, (uint8_t)totalLength);
}

void MQTT_unsubsribeTopic(char *topic)
{
    MQTT_unsubscribeTopic(topic);
}

void MQTT_disconnectBroker(void)
{
    uint16_t totalLength;
    if (!mqtt_prepare(MQTTdisconnect, 0U, &totalLength)) return;
    (void)ESP8266_sendMessage(mqttPacket, (uint8_t)totalLength);
}

void MQTT_pingReq(void)
{
    uint16_t totalLength;
    if (!mqtt_prepare(MQTTpingreq, 0U, &totalLength)) return;
    (void)ESP8266_sendMessage(mqttPacket, (uint8_t)totalLength);
}

/* Player 2 is an MQTT publisher only. These are compatibility stubs. */
void MQTTDataHandler(char *mqttTopic, char *mqttMsg)
{
    (void)mqttTopic;
    (void)mqttMsg;
}

void MQTTTimer(void)
{
    /* Not required by the blocking transmit-only Player 2 application. */
}
