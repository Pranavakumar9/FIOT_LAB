/**
 * @file main.c
 * @brief STM32F401RE Nucleo - Task 11.2: B1 Button Control with State Remembrance
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-14
 *
 * @details
 * Interfaces the on-board user blue push-button (B1 on PC13) to control the green
 * user LED (LD2 on PA5) on the NUCLEO-F401RE board:
 * - Implements a 30 ms software debouncing filter using HAL_GetTick().
 * - Toggles the LED exactly once per confirmed button release.
 * - Retains the active LED state indefinitely across button release.
 *
 * Nucleo Pin Configuration:
 * - PA5:  User LED LD2 (Active-HIGH)
 * - PC13: User Button B1 (Active-LOW, external 4.7k pull-up on board)
 */

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Hardware Pin Definitions */
#define LD2_LED_PORT            GPIOA
#define LD2_LED_PIN             GPIO_PIN_5

#define B1_BTN_PORT             GPIOC
#define B1_BTN_PIN              GPIO_PIN_13

/* Logic Level Definitions */
#define LED_ACTIVE_STATE        GPIO_PIN_SET    /* Active-HIGH: 3.3V turns LED ON */
#define LED_INACTIVE_STATE      GPIO_PIN_RESET  /* 0V turns LED OFF */
#define BTN_PRESSED_STATE       GPIO_PIN_RESET  /* Active-LOW: Button closes to GND */
#define BTN_RELEASED_STATE      GPIO_PIN_SET    /* Pulled up to 3.3V */

#define DEBOUNCE_TIME_MS        30U

/* Debounce State Machine */
typedef enum
{
    DEB_IDLE = 0,
    DEB_PRESS,
    DEB_HELD,
    DEB_RELEASE
} DebounceState_t;

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static bool Process_Button_Toggle_Event(void);
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

    /* Start with LED turned OFF */
    HAL_GPIO_WritePin(LD2_LED_PORT, LD2_LED_PIN, LED_INACTIVE_STATE);

    /* Infinite Superloop */
    while (1)
    {
        /* Poll the debounced button edge detector */
        if (Process_Button_Toggle_Event())
        {
            /* Clean button press cycle confirmed -> Toggle LD2 and retain state */
            HAL_GPIO_TogglePin(LD2_LED_PORT, LD2_LED_PIN);
        }
    }
}

/**
 * @brief  Non-blocking button debouncer and edge detector on PC13.
 * @retval true upon confirmed release, false otherwise.
 */
static bool Process_Button_Toggle_Event(void)
{
    static DebounceState_t fsm_state = DEB_IDLE;
    static uint32_t state_start_time = 0;
    bool event_dispatched = false;

    uint32_t current_time = HAL_GetTick();
    GPIO_PinState raw_pin = HAL_GPIO_ReadPin(B1_BTN_PORT, B1_BTN_PIN);

    switch (fsm_state)
    {
        case DEB_IDLE:
            if (raw_pin == BTN_PRESSED_STATE)
            {
                state_start_time = current_time;
                fsm_state = DEB_PRESS;
            }
            break;

        case DEB_PRESS:
            if (raw_pin != BTN_PRESSED_STATE)
            {
                fsm_state = DEB_IDLE; /* Glitch rejected */
            }
            else if ((current_time - state_start_time) >= DEBOUNCE_TIME_MS)
            {
                fsm_state = DEB_HELD;
            }
            break;

        case DEB_HELD:
            if (raw_pin == BTN_RELEASED_STATE)
            {
                state_start_time = current_time;
                fsm_state = DEB_RELEASE;
            }
            break;

        case DEB_RELEASE:
            if (raw_pin != BTN_RELEASED_STATE)
            {
                fsm_state = DEB_HELD; /* Bounced back */
            }
            else if ((current_time - state_start_time) >= DEBOUNCE_TIME_MS)
            {
                event_dispatched = true;
                fsm_state = DEB_IDLE;
            }
            break;

        default:
            fsm_state = DEB_IDLE;
            break;
    }

    return event_dispatched;
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
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4; /* 84 MHz */
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

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure PA5 as Output Push-Pull for LD2 */
    HAL_GPIO_WritePin(LD2_LED_PORT, LD2_LED_PIN, LED_INACTIVE_STATE);
    GPIO_InitStruct.Pin = LD2_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LD2_LED_PORT, &GPIO_InitStruct);

    /* Configure PC13 as Input with Pull-Up for Button B1 */
    GPIO_InitStruct.Pin = B1_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL; /* Nucleo board has external 4.7k pull-up R26 */
    HAL_GPIO_Init(B1_BTN_PORT, &GPIO_InitStruct);
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
