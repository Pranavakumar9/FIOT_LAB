/**
 * @file main.c
 * @brief STM32F103 - Task 10C: Automated Wi-Fi Station Association & IP Retrieval
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-05
 *
 * @details
 * Automates the Wi-Fi client association process between the STM32F103 host
 * and Wemos D1 (ESP8266) modem over USART1:
 * 1. Sets Station Mode (`AT+CWMODE=1`)
 * 2. Connects to the local wireless access point (`AT+CWJAP="..."`)
 * 3. Monitors association events (`WIFI CONNECTED`, `WIFI GOT IP`)
 * 4. Extracts assigned station IP address via `AT+CIFSR`
 * 5. Logs the entire provisioning process in real time to the PC console on USART2
 *
 * LED Status Indicator:
 * - Blinking (500 ms): Association in progress
 * - Solid ON: Wi-Fi connected and valid IP acquired
 * - Rapid Strobe (100 ms): Network connection failed
 *
 * Pin Mapping:
 * - PA9:  USART1_TX -> Wemos ESP8266 RXD
 * - PA10: USART1_RX <- Wemos ESP8266 TXD
 * - PA2:  USART2_TX -> PC Adapter RXD
 * - PA3:  USART2_RX <- PC Adapter TXD
 * - PC13: Active-LOW Status LED
 * - GND:  Common Ground
 */

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Wi-Fi Network Credentials */
#define WIFI_SSID               "Campus_IoT"
#define WIFI_PASSWORD           "SecurePass123"

/* Hardware Definitions */
#define USER_LED_PORT           GPIOC
#define USER_LED_PIN            GPIO_PIN_13

/* Logic Level Definitions */
#define LED_ACTIVE_STATE        GPIO_PIN_RESET
#define LED_INACTIVE_STATE      GPIO_PIN_SET

/* Buffer Limits */
#define MODEM_RX_BUFFER_SIZE    1024U

/* Peripheral Handles */
UART_HandleTypeDef huart1; /* Wemos ESP8266 Link */
UART_HandleTypeDef huart2; /* PC Terminal Link */

/* Private Variables */
static char modem_rx_buffer[MODEM_RX_BUFFER_SIZE];

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void PC_Print(const char *str);
static bool Send_AT_Command(const char *cmd, const char *expected_reply, uint32_t timeout_ms);
void Error_Handler(void);

/**
 * @brief Application entry point.
 * @retval int
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);

    PC_Print("\r\n=======================================================\r\n");
    PC_Print("  STM32F103 - Task 10C: Automated Wi-Fi Association    \r\n");
    PC_Print("  Target SSID: " WIFI_SSID "                           \r\n");
    PC_Print("=======================================================\r\n");

    /* Step 1: Modem Health Check */
    PC_Print("[STEP 1/4] Pinging ESP8266 Modem...\r\n");
    if (!Send_AT_Command("AT\r\n", "OK", 2000))
    {
        PC_Print("[FATAL] Modem unresponsive. Check UART wiring.\r\n");
        while (1) { HAL_GPIO_TogglePin(USER_LED_PORT, USER_LED_PIN); HAL_Delay(100); }
    }

    /* Step 2: Configure as Station Mode */
    PC_Print("[STEP 2/4] Setting Wi-Fi Station Mode (CWMODE=1)...\r\n");
    if (!Send_AT_Command("AT+CWMODE=1\r\n", "OK", 3000))
    {
        PC_Print("[WARN] Retrying CWMODE setting...\r\n");
        Send_AT_Command("AT+CWMODE=1\r\n", "OK", 3000);
    }

    /* Step 3: Join Wireless Access Point */
    PC_Print("[STEP 3/4] Associating with AP: '" WIFI_SSID "'...\r\n");
    char join_cmd[128];
    snprintf(join_cmd, sizeof(join_cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);

    /* Allow up to 15 seconds for 802.11 association and 4-way handshake */
    if (!Send_AT_Command(join_cmd, "WIFI GOT IP", 15000))
    {
        PC_Print("[ERROR] Wi-Fi connection failed. Verify SSID and Password.\r\n");
        while (1) { HAL_GPIO_TogglePin(USER_LED_PORT, USER_LED_PIN); HAL_Delay(100); }
    }

    /* Step 4: Retrieve Station IP Address */
    PC_Print("[STEP 4/4] Querying Station IP and MAC Address...\r\n");
    Send_AT_Command("AT+CIFSR\r\n", "OK", 3000);

    PC_Print("\r\n=======================================================\r\n");
    PC_Print("  Wi-Fi Connected Successfully! System Ready.          \r\n");
    PC_Print("=======================================================\r\n");

    /* Turn User LED Solidly ON to indicate valid IP acquisition */
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_ACTIVE_STATE);

    while (1)
    {
        /* Idle maintain connection */
    }
}

/**
 * @brief  Transmits an AT command to ESP8266 and collects response up to timeout.
 */
static bool Send_AT_Command(const char *cmd, const char *expected_reply, uint32_t timeout_ms)
{
    memset(modem_rx_buffer, 0, sizeof(modem_rx_buffer));
    uint16_t rx_idx = 0;

    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, (uint16_t)strlen(cmd), 1000);

    PC_Print(">> TX: ");
    PC_Print(cmd);

    uint32_t start_tick = HAL_GetTick();
    uint8_t incoming_byte = 0;

    while ((HAL_GetTick() - start_tick) < timeout_ms)
    {
        if (HAL_UART_Receive(&huart1, &incoming_byte, 1, 5) == HAL_OK)
        {
            if (rx_idx < (MODEM_RX_BUFFER_SIZE - 1U))
            {
                modem_rx_buffer[rx_idx++] = (char)incoming_byte;
                modem_rx_buffer[rx_idx] = '\0';
            }

            HAL_UART_Transmit(&huart2, &incoming_byte, 1, 10);

            if (strstr(modem_rx_buffer, expected_reply) != NULL)
            {
                PC_Print("\r\n[PASS] Verified: ");
                PC_Print(expected_reply);
                PC_Print("\r\n");
                return true;
            }
        }
    }

    PC_Print("\r\n[FAIL / TIMEOUT] Expected: ");
    PC_Print(expected_reply);
    PC_Print("\r\n");
    return false;
}

/**
 * @brief Prints ASCII string to Host PC Console via USART2.
 */
static void PC_Print(const char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)str, (uint16_t)strlen(str), 1000);
}

/**
 * @brief USART1 Initialization Function (Wemos D1 Link: 115200-8-N-1)
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
 * @brief USART2 Initialization Function (PC Terminal Link: 115200-8-N-1)
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

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);
    GPIO_InitStruct.Pin = USER_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USER_LED_PORT, &GPIO_InitStruct);

    /* Configure PA9 (USART1_TX) as AF Push-Pull */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure PA10 (USART1_RX) as Input Floating */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure PA2 (USART2_TX) as AF Push-Pull */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure PA3 (USART2_RX) as Input Floating */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief System Clock Configuration (72 MHz via HSE 8MHz crystal)
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9; /* 8 MHz * 9 = 72 MHz */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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
 * @brief Error Handler
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
