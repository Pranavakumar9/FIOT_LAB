/**
 * @file main.c
 * @brief STM32F103 Blue Pill - Task 8C: Non-Blocking Short vs Long Press Duration Detection
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-09-18
 *
 * @details
 * Distinguishes between short and long physical button presses on PC14 using
 * non-blocking SysTick timestamps via HAL_GetTick():
 *
 * Interaction Rules:
 * 1. Short Press (< 1000 ms):
 *    - If system is currently in Blinking Mode -> Exits blinking mode and keeps LED OFF.
 *    - If system is in Static Mode -> Toggles the LED state (ON <-> OFF).
 * 2. Long Press (>= 1000 ms):
 *    - Enters Continuous Blinking Mode (LED toggles every 500 ms non-blockingly).
 * 3. Next Short Press:
 *    - Immediately halts blinking mode and sets LED to extinguished state.
 *
 * System Timebase:
 * - SysTick configured for 1.0 ms tick rate.
 * - Non-blocking timer checks ensure CPU is never locked in busy-wait delay loops.
 *
 * Pin Mapping:
 * - PC13: Active-LOW User LED (Cathode to PC13 via 510R resistor, Anode to 3.3V)
 * - PC14: Active-LOW Tactile Push-Button (Terminal A to PC14, Terminal B to GND)
 * - 3.3V / GND: Power Rails
 */

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* Hardware Pin Definitions */
#define USER_LED_PORT           GPIOC
#define USER_LED_PIN            GPIO_PIN_13

#define USER_BTN_PORT           GPIOC
#define USER_BTN_PIN            GPIO_PIN_14

/* Logic Level Definitions for Active-LOW Components */
#define LED_ACTIVE_STATE        GPIO_PIN_RESET  /* 0V drives current through LED */
#define LED_INACTIVE_STATE      GPIO_PIN_SET    /* 3.3V halts current flow */
#define BTN_PRESSED_STATE       GPIO_PIN_RESET  /* Switch closes to GND */
#define BTN_RELEASED_STATE      GPIO_PIN_SET    /* Pulled up to 3.3V by internal resistor */

/* Timing Thresholds (in milliseconds) */
#define DEBOUNCE_TIME_MS        30U             /* Mechanical bounce settling window */
#define LONG_PRESS_THRESHOLD_MS 1000U           /* Duration threshold separating short from long */
#define BLINK_CADENCE_MS        500U            /* Non-blocking LED toggle cadence */

/* Event Classification */
typedef enum
{
    PRESS_EVENT_NONE = 0,
    PRESS_EVENT_SHORT,                          /* Press duration < 1000 ms */
    PRESS_EVENT_LONG                            /* Press duration >= 1000 ms */
} PressEvent_t;

/* System Operating State */
typedef enum
{
    SYSTEM_MODE_STATIC = 0,                     /* Manual toggling via short press */
    SYSTEM_MODE_BLINKING                        /* Periodic 500ms toggling via long press */
} SystemMode_t;

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static PressEvent_t Check_Button_Press_Duration(void);
static void Update_LED_Blink_Engine(SystemMode_t mode);
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

    /* Initialize PC13 (Output) and PC14 (Input Pull-Up) */
    MX_GPIO_Init();

    /* Initialize system state */
    SystemMode_t current_mode = SYSTEM_MODE_STATIC;
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);

    /* Infinite Superloop */
    while (1)
    {
        /* Evaluate button duration without blocking */
        PressEvent_t event = Check_Button_Press_Duration();

        if (event == PRESS_EVENT_SHORT)
        {
            if (current_mode == SYSTEM_MODE_BLINKING)
            {
                /* Short press cancels active blinking mode and turns LED OFF */
                current_mode = SYSTEM_MODE_STATIC;
                HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);
            }
            else
            {
                /* In static mode, short press toggles LED once */
                HAL_GPIO_TogglePin(USER_LED_PORT, USER_LED_PIN);
            }
        }
        else if (event == PRESS_EVENT_LONG)
        {
            /* Long press switches system into continuous blinking mode */
            current_mode = SYSTEM_MODE_BLINKING;
        }

        /* Non-blocking LED service routine */
        Update_LED_Blink_Engine(current_mode);
    }
}

/**
 * @brief  Samples PC14, applies debouncing, and measures press duration upon release.
 * @retval PRESS_EVENT_NONE, PRESS_EVENT_SHORT, or PRESS_EVENT_LONG.
 */
static PressEvent_t Check_Button_Press_Duration(void)
{
    typedef enum
    {
        DET_IDLE = 0,
        DET_DEBOUNCE_PRESS,
        DET_HELD,
        DET_DEBOUNCE_RELEASE
    } DetState_t;

    static DetState_t state = DET_IDLE;
    static uint32_t press_start_tick = 0;
    static uint32_t debounce_start_tick = 0;
    PressEvent_t detected_event = PRESS_EVENT_NONE;

    uint32_t now = HAL_GetTick();
    GPIO_PinState pin_val = HAL_GPIO_ReadPin(USER_BTN_PORT, USER_BTN_PIN);

    switch (state)
    {
        case DET_IDLE:
            if (pin_val == BTN_PRESSED_STATE)
            {
                debounce_start_tick = now;
                state = DET_DEBOUNCE_PRESS;
            }
            break;

        case DET_DEBOUNCE_PRESS:
            if (pin_val != BTN_PRESSED_STATE)
            {
                state = DET_IDLE; /* Glitch ignored */
            }
            else if ((now - debounce_start_tick) >= DEBOUNCE_TIME_MS)
            {
                /* Stable press detected; record official press start timestamp */
                press_start_tick = now;
                state = DET_HELD;
            }
            break;

        case DET_HELD:
            if (pin_val == BTN_RELEASED_STATE)
            {
                debounce_start_tick = now;
                state = DET_DEBOUNCE_RELEASE;
            }
            break;

        case DET_DEBOUNCE_RELEASE:
            if (pin_val != BTN_RELEASED_STATE)
            {
                state = DET_HELD; /* Bounced back to ground */
            }
            else if ((now - debounce_start_tick) >= DEBOUNCE_TIME_MS)
            {
                /* Button release confirmed. Calculate elapsed press duration */
                uint32_t press_duration = now - press_start_tick;

                if (press_duration >= LONG_PRESS_THRESHOLD_MS)
                {
                    detected_event = PRESS_EVENT_LONG;
                }
                else
                {
                    detected_event = PRESS_EVENT_SHORT;
                }

                state = DET_IDLE;
            }
            break;

        default:
            state = DET_IDLE;
            break;
    }

    return detected_event;
}

/**
 * @brief  Drives the non-blocking 500 ms periodic blink when in blinking mode.
 * @param  mode Current system mode (STATIC or BLINKING).
 */
static void Update_LED_Blink_Engine(SystemMode_t mode)
{
    static uint32_t last_toggle_tick = 0;

    if (mode == SYSTEM_MODE_BLINKING)
    {
        uint32_t current_tick = HAL_GetTick();
        if ((current_tick - last_toggle_tick) >= BLINK_CADENCE_MS)
        {
            last_toggle_tick = current_tick;
            HAL_GPIO_TogglePin(USER_LED_PORT, USER_LED_PIN);
        }
    }
}

/**
 * @brief System Clock Configuration (72 MHz via HSE 8MHz crystal)
 * @retval None
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
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Output Push-Pull on PC13 */
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);
    GPIO_InitStruct.Pin = USER_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USER_LED_PORT, &GPIO_InitStruct);

    /* Input Pull-Up on PC14 */
    GPIO_InitStruct.Pin = USER_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(USER_BTN_PORT, &GPIO_InitStruct);
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
