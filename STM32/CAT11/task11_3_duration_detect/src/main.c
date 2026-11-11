/**
 * @file main.c
 * @brief STM32F401RE Nucleo - Task 11.3: Non-Blocking Short vs Long Press Duration Detection
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-14
 *
 * @details
 * Distinguishes between short and long physical presses of user button B1 (PC13)
 * on the NUCLEO-F401RE development board using HAL_GetTick():
 *
 * Operational Rules:
 * 1. Short Press (< 1000 ms):
 *    - In Static Mode: Toggles the user LED (LD2 on PA5) once.
 *    - In Blinking Mode: Halts blinking and extinguishes the LED.
 * 2. Long Press (>= 1000 ms):
 *    - Activates Continuous Blinking Mode (LD2 toggles every 500 ms non-blockingly).
 * 3. Subsequent Actions:
 *    - Next short press stops blinking.
 *    - Next long press toggles blinking mode on/off.
 *
 * Nucleo Pin Configuration:
 * - PA5:  User LED LD2 (Active-HIGH)
 * - PC13: User Button B1 (Active-LOW, external 4.7k pull-up R26)
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
#define LED_ACTIVE_STATE        GPIO_PIN_SET    /* Active-HIGH: 3.3V illuminates LD2 */
#define LED_INACTIVE_STATE      GPIO_PIN_RESET  /* 0V extinguishes LD2 */
#define BTN_PRESSED_STATE       GPIO_PIN_RESET  /* Active-LOW: Button closes to GND */
#define BTN_RELEASED_STATE      GPIO_PIN_SET    /* Pulled up to 3.3V */

/* Timing Thresholds */
#define DEBOUNCE_TIME_MS        30U
#define LONG_PRESS_THRESHOLD_MS 1000U
#define BLINK_INTERVAL_MS       500U

/* Event & Mode Enumerations */
typedef enum
{
    EVENT_NONE = 0,
    EVENT_SHORT_PRESS,
    EVENT_LONG_PRESS
} PressEvent_t;

typedef enum
{
    MODE_STATIC = 0,
    MODE_BLINKING
} OperatingMode_t;

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static PressEvent_t Evaluate_Button_Press_Duration(void);
static void Service_LED_Blink_Engine(OperatingMode_t mode);
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

    OperatingMode_t current_mode = MODE_STATIC;
    HAL_GPIO_WritePin(LD2_LED_PORT, LD2_LED_PIN, LED_INACTIVE_STATE);

    /* Infinite Superloop */
    while (1)
    {
        PressEvent_t event = Evaluate_Button_Press_Duration();

        if (event == EVENT_SHORT_PRESS)
        {
            if (current_mode == MODE_BLINKING)
            {
                /* Short press exits active blinking mode and turns LED OFF */
                current_mode = MODE_STATIC;
                HAL_GPIO_WritePin(LD2_LED_PORT, LD2_LED_PIN, LED_INACTIVE_STATE);
            }
            else
            {
                /* In static mode, short press toggles LED */
                HAL_GPIO_TogglePin(LD2_LED_PORT, LD2_LED_PIN);
            }
        }
        else if (event == EVENT_LONG_PRESS)
        {
            /* Long press toggles blinking mode on or off */
            if (current_mode == MODE_STATIC)
            {
                current_mode = MODE_BLINKING;
            }
            else
            {
                current_mode = MODE_STATIC;
                HAL_GPIO_WritePin(LD2_LED_PORT, LD2_LED_PIN, LED_INACTIVE_STATE);
            }
        }

        /* Non-blocking LED periodic engine */
        Service_LED_Blink_Engine(current_mode);
    }
}

/**
 * @brief  Samples PC13, applies debouncing, and measures press duration upon release.
 * @retval EVENT_NONE, EVENT_SHORT_PRESS, or EVENT_LONG_PRESS.
 */
static PressEvent_t Evaluate_Button_Press_Duration(void)
{
    typedef enum
    {
        STATE_IDLE = 0,
        STATE_DEBOUNCE_PRESS,
        STATE_HELD,
        STATE_DEBOUNCE_RELEASE
    } DetState_t;

    static DetState_t fsm = STATE_IDLE;
    static uint32_t press_start_tick = 0;
    static uint32_t debounce_start_tick = 0;
    PressEvent_t result_event = EVENT_NONE;

    uint32_t now = HAL_GetTick();
    GPIO_PinState pin_val = HAL_GPIO_ReadPin(B1_BTN_PORT, B1_BTN_PIN);

    switch (fsm)
    {
        case STATE_IDLE:
            if (pin_val == BTN_PRESSED_STATE)
            {
                debounce_start_tick = now;
                fsm = STATE_DEBOUNCE_PRESS;
            }
            break;

        case STATE_DEBOUNCE_PRESS:
            if (pin_val != BTN_PRESSED_STATE)
            {
                fsm = STATE_IDLE;
            }
            else if ((now - debounce_start_tick) >= DEBOUNCE_TIME_MS)
            {
                press_start_tick = now;
                fsm = STATE_HELD;
            }
            break;

        case STATE_HELD:
            if (pin_val == BTN_RELEASED_STATE)
            {
                debounce_start_tick = now;
                fsm = STATE_DEBOUNCE_RELEASE;
            }
            break;

        case STATE_DEBOUNCE_RELEASE:
            if (pin_val != BTN_RELEASED_STATE)
            {
                fsm = STATE_HELD;
            }
            else if ((now - debounce_start_tick) >= DEBOUNCE_TIME_MS)
            {
                uint32_t duration = now - press_start_tick;

                if (duration >= LONG_PRESS_THRESHOLD_MS)
                {
                    result_event = EVENT_LONG_PRESS;
                }
                else
                {
                    result_event = EVENT_SHORT_PRESS;
                }

                fsm = STATE_IDLE;
            }
            break;

        default:
            fsm = STATE_IDLE;
            break;
    }

    return result_event;
}

/**
 * @brief  Executes non-blocking 500 ms periodic blink when in blinking mode.
 */
static void Service_LED_Blink_Engine(OperatingMode_t mode)
{
    static uint32_t last_blink_tick = 0;

    if (mode == MODE_BLINKING)
    {
        uint32_t now = HAL_GetTick();
        if ((now - last_blink_tick) >= BLINK_INTERVAL_MS)
        {
            last_blink_tick = now;
            HAL_GPIO_TogglePin(LD2_LED_PORT, LD2_LED_PIN);
        }
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

    /* Configure PC13 as Input for Button B1 */
    GPIO_InitStruct.Pin = B1_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
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
