/**
 * @file main.c
 * @brief STM32F103 Blue Pill - Task 8A: Direct GPIO Input/Output Polling Interface
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-09-18
 *
 * @details
 * Configures PC13 as a push-pull digital output (driving the active-LOW user LED)
 * and PC14 as a digital input with internal pull-up (connected to a tactile push-button).
 * The main superloop continuously polls the state of PC14 and drives PC13 directly
 * so that pressing the button illuminates the LED and releasing it extinguishes the LED.
 *
 * Pin Mapping:
 * - PC13: Active-LOW User LED (Cathode to PC13 via 510R resistor, Anode to 3.3V)
 * - PC14: Active-LOW Tactile Push-Button (Terminal A to PC14, Terminal B to GND)
 * - 3.3V / GND: Power Rails
 */

#include "stm32f1xx_hal.h"

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
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock to 72 MHz using HSE / PLL */
    SystemClock_Config();

    /* Initialize all configured GPIO pins */
    MX_GPIO_Init();

    /* Ensure LED starts in the extinguished (OFF) state */
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);

    /* Infinite Superloop */
    while (1)
    {
        /* Sample the momentary raw logic state on PC14 */
        GPIO_PinState button_state = HAL_GPIO_ReadPin(USER_BTN_PORT, USER_BTN_PIN);

        /* Directly translate button state to LED drive */
        if (button_state == BTN_PRESSED_STATE)
        {
            /* Button pressed (GND level detected) -> Turn User LED ON */
            HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_ACTIVE_STATE);
        }
        else
        {
            /* Button released (Pull-up level detected) -> Turn User LED OFF */
            HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);
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

    /* Initializes the RCC Oscillators according to the specified parameters */
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

    /* Initializes the CPU, AHB and APB buses clocks */
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

    /* Enable GPIOC Clock */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure PC13 as Output Push-Pull for User LED */
    HAL_GPIO_WritePin(USER_LED_PORT, USER_LED_PIN, LED_INACTIVE_STATE);
    GPIO_InitStruct.Pin = USER_LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USER_LED_PORT, &GPIO_InitStruct);

    /* Configure PC14 as Input with Internal Pull-Up for Push-Button */
    GPIO_InitStruct.Pin = USER_BTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(USER_BTN_PORT, &GPIO_InitStruct);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* Trap CPU on clock or initialization fault */
    }
}
