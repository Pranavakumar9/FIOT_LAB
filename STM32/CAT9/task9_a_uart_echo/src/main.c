/**
 * @file main.c
 * @brief STM32F103 Blue Pill - Task 9A: USART1 Bidirectional Character Echo Console
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-09-28
 *
 * @details
 * Configures USART1 peripheral on PA9 (TX) and PA10 (RX) for asynchronous serial
 * communications at 9600 baud, 8 data bits, no parity, and 1 stop bit (9600-8-N-1).
 * Transmits a boot initialization banner to the host PC, then enters an interactive
 * full-duplex loop where incoming characters are immediately mirrored back to the sender.
 * Pin PC13 is configured as an active-LOW heartbeat indicator.
 *
 * Pin Mapping:
 * - PA9:  USART1_TX -> Host USB-to-TTL Adapter RXD
 * - PA10: USART1_RX <- Host USB-to-TTL Adapter TXD
 * - PC13: Active-LOW Status LED
 * - GND:  Common Ground
 */

#include "stm32f1xx_hal.h"
#include <string.h>

/* Hardware Definitions */
#define USER_LED_PORT       GPIOC
#define USER_LED_PIN        GPIO_PIN_13

/* Logic Level Definitions */
#define LED_ACTIVE_STATE    GPIO_PIN_RESET  /* 0V illuminates LED */
#define LED_INACTIVE_STATE  GPIO_PIN_SET    /* 3.3V extinguishes LED */

/* Private Variables */
UART_HandleTypeDef huart1;

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void UART_SendString(const char *str);
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

    /* Initialize GPIO (PC13 LED) and USART1 (PA9/PA10) */
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    /* Transmit initial boot banner */
    UART_SendString("\r\n========================================\r\n");
    UART_SendString("  STM32F103 Blue Pill USART1 Echo Test  \r\n");
    UART_SendString("  Baud Rate: 9600-8-N-1 | Core: 72 MHz  \r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("Type any character in the terminal to echo:\r\n\r\n");

    uint8_t rx_byte = 0;

    /* Infinite Echo Loop */
    while (1)
    {
        /* Poll USART1 receiver with 100 ms timeout */
        if (HAL_UART_Receive(&huart1, &rx_byte, 1, 100) == HAL_OK)
        {
            /* Instantly mirror the received byte back to host transmitter */
            HAL_UART_Transmit(&huart1, &rx_byte, 1, 100);

            /* If user hit Carriage Return, append Line Feed for terminal alignment */
            if (rx_byte == '\r')
            {
                uint8_t lf = '\n';
                HAL_UART_Transmit(&huart1, &lf, 1, 50);
            }

            /* Pulse the LED briefly to visually register serial traffic */
            HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_ACTIVE_STATE);
            HAL_Delay(5);
            HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);
        }
    }
}

/**
 * @brief Transmits a null-terminated string over USART1.
 * @param str Pointer to constant ASCII string.
 */
static void UART_SendString(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), 1000);
}

/**
 * @brief USART1 Initialization Function (9600-8-N-1)
 */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
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
 * @brief GPIO Initialization Function
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* Configure PC13 as Output Push-Pull for User LED */
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);
    GPIO_InitStruct.Pin = USER_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USER_LED_PORT, &GPIO_InitStruct);

    /* Configure PA9 as Alternate Function Push-Pull for USART1_TX */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure PA10 as Input Floating for USART1_RX */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
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
