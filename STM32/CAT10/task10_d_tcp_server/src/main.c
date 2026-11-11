/**
 * @file main.c
 * @brief STM32F103 - Task 10D: Multi-Connection TCP Web Server & 2-Step CIPSEND HTTP Response
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-05
 *
 * @details
 * Configures the Wemos D1 (ESP8266) modem as an embedded HTTP web server on TCP port 80:
 * 1. Enables multiple connection handling: `AT+CIPMUX=1`
 * 2. Starts TCP listening server: `AT+CIPSERVER=1,80`
 * 3. Monitors incoming HTTP GET requests signaled via `+IPD,<link_id>,<len>:<data>`
 * 4. Extracts the active client socket link ID
 * 5. Executes the mandatory 2-step `AT+CIPSEND` transaction:
 *    - Step 1: Dispatches `AT+CIPSEND=<link_id>,<length>\r\n` and waits for `>` prompt
 *    - Step 2: Streams the formatted HTTP/1.1 200 OK response with accurate `Content-Length`
 * 6. Closes the socket cleanly via `AT+CIPCLOSE=<link_id>`
 *
 * Pin Mapping:
 * - PA9:  USART1_TX -> Wemos ESP8266 RXD
 * - PA10: USART1_RX <- Wemos ESP8266 TXD
 * - PA2:  USART2_TX -> PC Adapter RXD
 * - PA3:  USART2_RX <- PC Adapter TXD
 * - PC13: Active-LOW Status LED (Toggles on HTTP request service)
 * - GND:  Common Ground
 */

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Hardware Definitions */
#define USER_LED_PORT           GPIOC
#define USER_LED_PIN            GPIO_PIN_13

/* Logic Level Definitions */
#define LED_ACTIVE_STATE        GPIO_PIN_RESET
#define LED_INACTIVE_STATE      GPIO_PIN_SET

/* Buffer Limits */
#define MODEM_BUF_SIZE          1024U
#define HTTP_RESP_BUF_SIZE      512U

/* Peripheral Handles */
UART_HandleTypeDef huart1; /* Wemos ESP8266 Link */
UART_HandleTypeDef huart2; /* PC Terminal Link */

/* Private Variables */
static char modem_rx_buffer[MODEM_BUF_SIZE];
static char http_payload[HTTP_RESP_BUF_SIZE];

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void PC_Print(const char *str);
static bool Send_AT_Command(const char *cmd, const char *expected_reply, uint32_t timeout_ms);
static void Handle_HTTP_Client(int link_id);
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
    PC_Print("  STM32F103 - Task 10D: Embedded TCP Web Server (Pt 80)\r\n");
    PC_Print("=======================================================\r\n");

    HAL_Delay(1000);

    /* Step 1: Enable Multi-Connection Mode */
    PC_Print("[SETUP] Enabling Multiple Connections (CIPMUX=1)...\r\n");
    Send_AT_Command("AT+CIPMUX=1\r\n", "OK", 3000);

    /* Step 2: Start TCP Server on Port 80 */
    PC_Print("[SETUP] Starting TCP Web Server on Port 80 (CIPSERVER=1,80)...\r\n");
    if (!Send_AT_Command("AT+CIPSERVER=1,80\r\n", "OK", 3000))
    {
        PC_Print("[WARN] Retrying CIPSERVER initialization...\r\n");
        Send_AT_Command("AT+CIPSERVER=1,80\r\n", "OK", 3000);
    }

    PC_Print("[READY] Web Server listening for HTTP clients on port 80!\r\n");
    PC_Print("Point your browser or Node-RED client to http://<ESP_IP>/\r\n\r\n");

    uint16_t rx_idx = 0;
    uint8_t ch = 0;

    /* Web Server Request Monitoring Loop */
    while (1)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1, 10) == HAL_OK)
        {
            if (rx_idx < (MODEM_BUF_SIZE - 1U))
            {
                modem_rx_buffer[rx_idx++] = (char)ch;
                modem_rx_buffer[rx_idx] = '\0';
            }

            /* Detect start of incoming client request (+IPD) */
            char *ipd_ptr = strstr(modem_rx_buffer, "+IPD,");
            if (ipd_ptr != NULL)
            {
                /* Format: +IPD,<link_id>,<len>: */
                int link_id = -1;
                if (sscanf(ipd_ptr, "+IPD,%d,", &link_id) == 1)
                {
                    PC_Print("\r\n[HTTP] Inbound connection detected! Link ID: ");
                    char id_str[16];
                    snprintf(id_str, sizeof(id_str), "%d\r\n", link_id);
                    PC_Print(id_str);

                    /* Wait briefly for remainder of HTTP GET request to arrive */
                    HAL_Delay(50);

                    /* Serve HTML response to the connected client */
                    Handle_HTTP_Client(link_id);

                    /* Clear buffer for subsequent requests */
                    memset(modem_rx_buffer, 0, sizeof(modem_rx_buffer));
                    rx_idx = 0;
                }
            }
        }
    }
}

/**
 * @brief Constructs and transmits an HTTP/1.1 200 OK webpage via 2-Step CIPSEND.
 * @param link_id Client socket connection identifier (0 to 4).
 */
static void Handle_HTTP_Client(int link_id)
{
    /* Toggle LED to visually confirm request service */
    HAL_GPIO_TogglePin(USER_LED_PORT, USER_LED_PIN);

    /* Construct HTML Body */
    const char *html_body =
        "<!DOCTYPE html><html><head><title>STM32 IoT Server</title>"
        "<meta http-equiv='refresh' content='5'></head>"
        "<body style='font-family:sans-serif; text-align:center; padding:40px;'>"
        "<h1 style='color:#0f2b46;'>STM32 + Wemos ESP8266 Web Server</h1>"
        "<p style='color:#27ae60; font-weight:bold; font-size:18px;'>Status: ONLINE &amp; RESPONSIVE</p>"
        "<p>Platform: STM32F103 Blue Pill @ 72 MHz</p>"
        "<p>Port: TCP 80 | Protocol: HTTP/1.1 200 OK</p>"
        "</body></html>";

    int body_length = (int)strlen(html_body);

    /* Construct Full HTTP Response Header + Body */
    snprintf(http_payload, sizeof(http_payload),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n"
             "%s",
             body_length, html_body);

    int total_length = (int)strlen(http_payload);

    /* Step 1 of CIPSEND: Transmit Length Command */
    char cipsend_cmd[64];
    snprintf(cipsend_cmd, sizeof(cipsend_cmd), "AT+CIPSEND=%d,%d\r\n", link_id, total_length);
    PC_Print(">> TX: ");
    PC_Print(cipsend_cmd);

    HAL_UART_Transmit(&huart1, (uint8_t *)cipsend_cmd, (uint16_t)strlen(cipsend_cmd), 1000);

    /* Wait for modem prompt ('>') up to 2 seconds */
    uint32_t prompt_start = HAL_GetTick();
    bool got_prompt = false;
    uint8_t resp_char = 0;

    while ((HAL_GetTick() - prompt_start) < 2000)
    {
        if (HAL_UART_Receive(&huart1, &resp_char, 1, 5) == HAL_OK)
        {
            if (resp_char == '>')
            {
                got_prompt = true;
                break;
            }
        }
    }

    if (got_prompt)
    {
        PC_Print("[CIPSEND] Prompt '>' received. Streaming HTTP response payload...\r\n");
        /* Step 2 of CIPSEND: Stream Raw Data Payload */
        HAL_UART_Transmit(&huart1, (uint8_t *)http_payload, (uint16_t)total_length, 2000);
        HAL_Delay(100);
    }
    else
    {
        PC_Print("[WARN] Timed out waiting for '>' prompt!\r\n");
    }

    /* Step 3: Close Client Connection */
    char cipclose_cmd[64];
    snprintf(cipclose_cmd, sizeof(cipclose_cmd), "AT+CIPCLOSE=%d\r\n", link_id);
    HAL_UART_Transmit(&huart1, (uint8_t *)cipclose_cmd, (uint16_t)strlen(cipclose_cmd), 1000);
    PC_Print("[HTTP] Client socket closed successfully.\r\n");
}

/**
 * @brief Transmits an AT command to ESP8266 and awaits confirmation string.
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
            if (rx_idx < (MODEM_BUF_SIZE - 1U))
            {
                modem_rx_buffer[rx_idx++] = (char)incoming_byte;
                modem_rx_buffer[rx_idx] = '\0';
            }

            HAL_UART_Transmit(&huart2, &incoming_byte, 1, 10);

            if (strstr(modem_rx_buffer, expected_reply) != NULL)
            {
                return true;
            }
        }
    }
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
