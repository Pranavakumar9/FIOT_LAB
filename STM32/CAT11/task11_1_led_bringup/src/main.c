/**
 * @file main.c
 * @brief STM32F401RE Nucleo - Task 11.1: On-Board User LED (LD2) Bring-Up
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-14
 *
 * @details
 * Configures the ARM Cortex-M4 NUCLEO-F401RE development board to blink
 * the on-board user LED (LD2) connected to pin PA5 at a fixed 500 ms cadence (1 Hz).
 *
 * Nucleo-64 Hardware Differences vs Blue Pill:
 * - MCU: STM32F401RET6 (Cortex-M4 with FPU @ 84 MHz)
 * - LED LD2: Connected to PA5 with a 330 ohm resistor.
 * - Polarity: ACTIVE-HIGH (PA5=SET turns LED ON; PA5=RESET turns LED OFF).
 *
 * Pin Mapping:
 * - PA5: User LED LD2 (Active-HIGH)
 */

#include "stm32f4xx_hal.h"

/* Hardware Pin Definitions */
#define LD2_LED_PORT            GPIOA
#define LD2_LED_PIN             GPIO_PIN_5

/* Logic Level Definitions for Active-HIGH LD2 */
#define LED_ACTIVE_STATE        GPIO_PIN_SET    /* 3.3V forward-biases LED */
#define LED_INACTIVE_STATE      GPIO_PIN_RESET  /* 0V extinguishes LED */

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void Error_Handler(void);

/**
 * @brief Application entry point.
 * @retval int
 */
int main(void)
{
    /* Reset of all peripherals, Initializes Flash interface and Systick */
    HAL_Init();

    /* Configure 84 MHz system clock for STM32F401RE */
    SystemClock_Config();

    /* Initialize GPIO Port A Pin 5 for Output Push-Pull */
    MX_GPIO_Init();

    /* Infinite Blink Loop */
    while (1)
    {
        /* Invert state of LD2 */
        HAL_GPIO_TogglePin(LD2_LED_PORT, LD2_LED_PIN);

        /* Wait 500 milliseconds */
        HAL_Delay(500);
    }
}

/**
 * @brief System Clock Configuration (84 MHz using HSI / PLL)
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
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4; /* (16 MHz / 16) * 336 / 4 = 84 MHz */
    RCC_OscInitStruct.PLL.PLLQ = 7;
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
 * @brief GPIO Initialization Function
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIOA Clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure PA5 as Output Push-Pull for LD2 */
    HAL_GPIO_WritePin(LD2_LED_PORT, LD2_LED_PIN, LED_INACTIVE_STATE);
    GPIO_InitStruct.Pin = LD2_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LD2_LED_PORT, &GPIO_InitStruct);
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
