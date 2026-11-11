/**
 * @file main.c
 * @brief STM32F103 Blue Pill - Task 8B: Event-Based Control with Software Debouncing
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-09-18
 *
 * @details
 * Implements a non-blocking debounce finite state machine (FSM) to eliminate
 * mechanical switch contact bounce on PC14. Whenever a confirmed press-and-release
 * event occurs, the on-board user LED on PC13 is toggled exactly once.
 *
 * Debounce Architecture:
 * - Sampling window: 30 ms stability confirmation
 * - Non-blocking timer: Uses HAL_GetTick() (SysTick 1ms timebase)
 * - Edge trigger: Confirmed transition from Pressed to Released toggles LED
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

/* Debounce Configuration Parameters */
#define DEBOUNCE_DELAY_MS       30U             /* 30 ms settling filter window */

/* Debounce State Machine Enumeration */
typedef enum
{
    BTN_FSM_IDLE = 0,           /* Waiting for initial press contact (PC14 falling edge) */
    BTN_FSM_DEBOUNCE_PRESS,     /* Verifying raw low level persists across debounce window */
    BTN_FSM_HELD,               /* Confirmed pressed; waiting for physical release */
    BTN_FSM_DEBOUNCE_RELEASE    /* Verifying release level persists before returning to IDLE */
} ButtonFSM_State_t;

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static bool Process_Button_Event(void);
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

    /* Start with LED turned OFF */
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);

    /* Infinite Superloop */
    while (1)
    {
        /* Poll the debouncer state machine */
        if (Process_Button_Event())
        {
            /* A clean, verified button press event completed -> Toggle User LED */
            HAL_GPIO_TogglePin(USER_LED_PORT, USER_LED_PIN);
        }
    }
}

/**
 * @brief  Non-blocking button debouncer and edge detector.
 * @retval true if a single confirmed button release event just occurred, false otherwise.
 */
static bool Process_Button_Event(void)
{
    static ButtonFSM_State_t fsm_state = BTN_FSM_IDLE;
    static uint32_t state_entry_time = 0;
    bool event_triggered = false;

    uint32_t current_time = HAL_GetTick();
    GPIO_PinState pin_raw = HAL_GPIO_ReadPin(USER_BTN_PORT, USER_BTN_PIN);

    switch (fsm_state)
    {
        case BTN_FSM_IDLE:
            /* Detect physical contact closure (active-LOW) */
            if (pin_raw == BTN_PRESSED_STATE)
            {
                state_entry_time = current_time;
                fsm_state = BTN_FSM_DEBOUNCE_PRESS;
            }
            break;

        case BTN_FSM_DEBOUNCE_PRESS:
            /* If signal glitches back to released within window, discard as false noise */
            if (pin_raw != BTN_PRESSED_STATE)
            {
                fsm_state = BTN_FSM_IDLE;
            }
            else if ((current_time - state_entry_time) >= DEBOUNCE_DELAY_MS)
            {
                /* Signal remained solidly LOW for >= 30ms; button is officially pressed */
                fsm_state = BTN_FSM_HELD;
            }
            break;

        case BTN_FSM_HELD:
            /* Detect start of physical release (switch opening) */
            if (pin_raw == BTN_RELEASED_STATE)
            {
                state_entry_time = current_time;
                fsm_state = BTN_FSM_DEBOUNCE_RELEASE;
            }
            break;

        case BTN_FSM_DEBOUNCE_RELEASE:
            /* If signal bounces back to pressed within window, keep holding */
            if (pin_raw != BTN_RELEASED_STATE)
            {
                fsm_state = BTN_FSM_HELD;
            }
            else if ((current_time - state_entry_time) >= DEBOUNCE_DELAY_MS)
            {
                /* Signal remained solidly HIGH for >= 30ms; release confirmed */
                event_triggered = true;
                fsm_state = BTN_FSM_IDLE;
            }
            break;

        default:
            fsm_state = BTN_FSM_IDLE;
            break;
    }

    return event_triggered;
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
