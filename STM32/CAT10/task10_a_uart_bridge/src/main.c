/**
 * @file main.c
 * @brief STM32F103 - Task 10A: Dual-UART Transparent Bridge & ESP8266 Modem Ping
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-05
 *
 * @details
 * Establishes a transparent bidirectional hardware bridge between:
 * - USART1 (PA9 TX, PA10 RX @ 115200 baud) connected to Wemos D1 (ESP8266) Wi-Fi Modem
 * - USART2 (PA2 TX, PA3 RX @ 115200 baud) connected to Host PC via USB-to-TTL Adapter
 *
 * Automatically issues the fundamental AT modem ping ("AT\r\n") upon boot,
 * validates the response ("OK\r\n"), and forwards all real-time bidirectional traffic
 * between the PC terminal and the ESP8266 coprocessor.
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
#include <stdbool.h>

/* Hardware Definitions */
#define USER_LED_PORT       GPIOC
#define USER_LED_PIN        GPIO_PIN_13

/* Logic Level Definitions */
#define LED_ACTIVE_STATE    GPIO_PIN_RESET
#define LED_INACTIVE_STATE  GPIO_PIN_SET

/* Peripheral Handles */
UART_HandleTypeDef huart1; /* Wemos ESP8266 Link */
UART_HandleTypeDef huart2; /* PC Terminal Link */

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void PC_Print(const char *str);
static void Modem_Send(const char *str);
void Error_Handler(void);

/**
 * @brief Application entry point.
 * @retval int
 */
int main(void)
{
    /* Reset of all peripherals, Initializes Flash interface and Systick */
    HAL_Init();

    /* Configure 72 MHz system clock */
    SystemClock_Config();

    /* Initialize Peripherals */
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    /* Turn off LED initially */
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);

    /* Output System Banner to PC Console */
    PC_Print("\r\n=======================================================\r\n");
    PC_Print("  STM32F103 - Task 10A: Dual-UART Bridge & Modem Ping  \r\n");
    PC_Print("  USART1: Wemos ESP8266 (115200) | USART2: PC (115200) \r\n");
    PC_Print("=======================================================\r\n");

    /* Automated Boot Ping: Transmit AT to Wemos */
    PC_Print("[STM32] Pinging ESP8266 Modem: Transmitting 'AT\\r\\n'...\r\n");
    Modem_Send("AT\r\n");

    uint8_t byte_from_pc = 0;
    uint8_t byte_from_modem = 0;

    /* Transparent Bridge Superloop */
    while (1)
    {
        /* 1. Forward data from PC Terminal (USART2) to ESP8266 Modem (USART1) */
        if (HAL_UART_Receive(&huart2, &byte_from_pc, 1, 1) == HAL_OK)
        {
            HAL_UART_Transmit(&huart1, &byte_from_pc, 1, 10);
            /* Pulse LED briefly to indicate outgoing host traffic */
            HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_ACTIVE_STATE);
        }

        /* 2. Forward data from ESP8266 Modem (USART1) to PC Terminal (USART2) */
        if (HAL_UART_Receive(&huart1, &byte_from_modem, 1, 1) == HAL_OK)
        {
            HAL_UART_Transmit(&huart2, &byte_from_modem, 1, 10);
            /* Turn off LED once modem traffic completes */
            HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);
        }
    }
}

/**
 * @brief Prints ASCII string to Host PC Console via USART2.
 */
static void PC_Print(const char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)str, (uint16_t)strlen(str), 1000);
}

/**
 * @brief Transmits ASCII command string to Wemos ESP8266 via USART1.
 */
static void Modem_Send(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), 1000);
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

    /* Configure PC13 as Output Push-Pull for User LED */
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
