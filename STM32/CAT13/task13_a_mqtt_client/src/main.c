/**
 * @file main.c
 * @brief STM32 NUCLEO-F401RE - Task 13A: ESP8266 AT MQTT 3.1.1 Client Bring-Up
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-27
 *
 * @details
 * Validates the custom ESPLib MQTT 3.1.1 protocol driver over an ESP8266 AT modem:
 * 1. Initializes USART2 (PA2 TX, PA3 RX) at 115200 baud for ESP8266 modem AT communications.
 * 2. Initializes USART1 (PA9 TX, PA10 RX) at 115200 baud for PC debug logging.
 * 3. Associates with local Wi-Fi access point in Station Mode (AT+CWMODE=1, AT+CWJAP).
 * 4. Establishes a TCP transport socket to public MQTT broker "broker.emqx.io" on port 1883.
 * 5. Generates and dispatches binary MQTT CONNECT packet (3.1.1 Clean Session, KeepAlive 500s).
 * 6. Subscribes to MQTT test topic "IOT" at QoS 0.
 * 7. Periodically publishes heartbeat JSON packet to "stm32/test/data" every 3 seconds.
 * 8. Strobe User LED (PA5 LD2) on each published telemetry frame.
 *
 * Pin Mapping:
 * - PA2:  USART2_TX -> ESP8266 RX (115200-8-N-1)
 * - PA3:  USART2_RX <- ESP8266 TX
 * - PA9:  USART1_TX -> PC Terminal (115200-8-N-1)
 * - PA10: USART1_RX <- PC Terminal
 * - PA5:  LD2 (Green User LED - Active-HIGH MQTT publication strobe)
 * - 3.3V / GND: Common system logic power rail
 */

#include "stm32f4xx_hal.h"
#include "ESPLib.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define WIFI_SSID       "LAB_IOT_WIFI"
#define WIFI_PASSWORD   "iotlab2026"
#define MQTT_BROKER     "broker.emqx.io"
#define MQTT_PORT       "1883"
#define MQTT_CLIENT_ID  "SNU_STM32_CAT13A"
#define MQTT_PUB_TOPIC  "stm32/test/data"
#define MQTT_SUB_TOPIC  "IOT"

/* Peripheral Handles */
UART_HandleTypeDef huart1; /* PC Debug Console */
UART_HandleTypeDef huart2; /* ESP8266 AT Interface */

/* Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
void Error_Handler(void);
static void Debug_Print(const char *msg);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    Debug_Print("\r\n=======================================================\r\n");
    Debug_Print("  STM32F401RE Task 13A: ESPLib MQTT Client Bring-Up    \r\n");
    Debug_Print("  Target Broker: broker.emqx.io:1883                   \r\n");
    Debug_Print("=======================================================\r\n");

    /* Step 1: Initialize ESP8266 in Station Mode */
    Debug_Print("[INFO] Initializing ESP8266 modem on USART2...\r\n");
    if (ESP8266_Init(StationMode, &huart2) != funcOk)
    {
        Debug_Print("[ERROR] ESP8266 initialization failed! Check wiring/baud.\r\n");
        Error_Handler();
    }
    Debug_Print("[OK] ESP8266 initialized successfully in Station Mode.\r\n");

    /* Step 2: Connect to Wi-Fi Access Point */
    Debug_Print("[INFO] Associating with Wi-Fi AP...\r\n");
    if (ESP8266_wifiConnect(WIFI_SSID, WIFI_PASSWORD) != funcOk)
    {
        Debug_Print("[ERROR] Wi-Fi connection failed! Check SSID/Password.\r\n");
        Error_Handler();
    }
    Debug_Print("[OK] Wi-Fi connected and DHCP IP obtained.\r\n");

    /* Step 3: Establish TCP Socket Connection to MQTT Broker */
    Debug_Print("[INFO] Opening TCP socket to broker.emqx.io:1883...\r\n");
    if (ESP8266_portConnect("TCP", MQTT_BROKER, MQTT_PORT) != funcOk)
    {
        Debug_Print("[ERROR] TCP connection to MQTT broker failed!\r\n");
        Error_Handler();
    }
    Debug_Print("[OK] TCP connection established to broker.emqx.io:1883.\r\n");

    /* Step 4: Dispatch MQTT CONNECT Packet */
    MQTT_InitTypeDef_t mqttSetting;
    mqttSetting.clientID = MQTT_CLIENT_ID;
    mqttSetting.username = NULL;
    mqttSetting.password = NULL;
    mqttSetting.keepAlive = 500U;
    mqttSetting.connectFlag = CleanSession;

    Debug_Print("[INFO] Dispatching MQTT 3.1.1 CONNECT packet...\r\n");
    MQTT_connectBroker(&mqttSetting);
    HAL_Delay(500);

    /* Step 5: Subscribe to Topic */
    Debug_Print("[INFO] Subscribing to topic 'IOT' (QoS 0)...\r\n");
    MQTT_subscribeTopic(MQTT_SUB_TOPIC, QoS0);
    HAL_Delay(500);
    Debug_Print("[READY] MQTT Client connected and active. Publishing heartbeats...\r\n\r\n");

    uint32_t seq = 0;
    char payload[96];
    char log_buf[128];

    while (1)
    {
        seq++;
        snprintf(payload, sizeof(payload), "{\"device\":\"STM32F401RE\",\"heartbeat\":%lu,\"status\":\"online\"}", (unsigned long)seq);

        /* Publish to MQTT topic */
        MQTT_publishTopic(MQTT_PUB_TOPIC, payload, QoS0);

        /* Log to PC debug console */
        snprintf(log_buf, sizeof(log_buf), "[PUB] Topic: %s | Payload: %s\r\n", MQTT_PUB_TOPIC, payload);
        Debug_Print(log_buf);

        /* Strobe Activity LED */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

        /* 3-second cycle */
        HAL_Delay(2900);
    }
}

/**
 * @brief Prints string to PC debug terminal via USART1.
 */
static void Debug_Print(const char *msg)
{
    if (msg != NULL)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 2000);
    }
}

/**
 * @brief USART1 Initialization Function (PC Debug Console @ 115200-8-N-1)
 */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief USART2 Initialization Function (ESP8266 Modem @ 115200-8-N-1)
 */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization Function
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure PA5 (LD2 LED) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief System Clock Configuration (HSI -> PLL -> 84 MHz)
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief SysTick Interrupt Handler - Servicing ESPLib MQTT Keep-Alive Timer
 */
void SysTick_Handler(void)
{
    MQTTTimer();
    HAL_IncTick();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(250);
    }
}
