/**
 * ==============================================================================
 * @file           : main.h
 * @brief          : Header for Non-Blocking Press Duration Classifier (Task 8C)
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT8 (Week 7 - 18/09/2026) - Task 8C: Short vs Long Press
 * @target         : STM32F103C8T6 (Blue Pill) @ 72 MHz (ARM Cortex-M3)
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#if defined(USE_HAL_DRIVER)
  #include "stm32f1xx_hal.h"
#else
  typedef enum {
      GPIO_PIN_RESET = 0U,
      GPIO_PIN_SET
  } GPIO_PinState;

  typedef enum {
      HAL_OK       = 0x00U,
      HAL_ERROR    = 0x01U,
      HAL_BUSY     = 0x02U,
      HAL_TIMEOUT  = 0x03U
  } HAL_StatusTypeDef;

  typedef struct {
      volatile uint32_t CRL;
      volatile uint32_t CRH;
      volatile uint32_t IDR;
      volatile uint32_t ODR;
      volatile uint32_t BSRR;
      volatile uint32_t BRR;
      volatile uint32_t LCKR;
  } GPIO_TypeDef;

  typedef struct {
      uint32_t Pin;
      uint32_t Mode;
      uint32_t Pull;
      uint32_t Speed;
  } GPIO_InitTypeDef;

  #define GPIOC               ((GPIO_TypeDef *)0x40011000UL)
  #define GPIO_PIN_13         ((uint16_t)0x2000U)
  #define GPIO_PIN_14         ((uint16_t)0x4000U)
  #define GPIO_MODE_INPUT     0x00000000U
  #define GPIO_MODE_OUTPUT_PP 0x00000001U
  #define GPIO_NOPULL         0x00000000U
  #define GPIO_PULLUP         0x00000001U
  #define GPIO_SPEED_FREQ_LOW 0x00000002U

  static inline HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }
  static inline void __HAL_RCC_GPIOC_CLK_ENABLE(void) {}
  static inline void __HAL_RCC_AFIO_CLK_ENABLE(void) {}
  static inline void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) { (void)GPIOx; (void)GPIO_Init; }
  static inline GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) { (void)GPIOx; (void)GPIO_Pin; return GPIO_PIN_SET; }
  static inline void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) { (void)GPIOx; (void)GPIO_Pin; (void)PinState; }
  static inline void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) { (void)GPIOx; (void)GPIO_Pin; }
  static inline uint32_t HAL_GetTick(void) { return 0U; }
  static inline void HAL_Delay(uint32_t Delay) { (void)Delay; }
#endif

/* Hardware Pin Assignments */
#define USER_LED_PORT             GPIOC
#define USER_LED_PIN              GPIO_PIN_13

#define USER_BTN_PORT             GPIOC
#define USER_BTN_PIN              GPIO_PIN_14

#define LED_ACTIVE_ON             GPIO_PIN_RESET
#define LED_ACTIVE_OFF            GPIO_PIN_SET
#define BTN_PRESSED_STATE         GPIO_PIN_RESET
#define BTN_RELEASED_STATE        GPIO_PIN_SET

/* Timing Constants (ms) */
#define DEBOUNCE_GUARD_MS         25U
#define LONG_PRESS_THRESHOLD_MS   1000U  /* >= 1000 ms classified as Long Press */
#define BLINK_HALF_PERIOD_MS      250U   /* 500 ms total period (2 Hz blink) */

/* Event Classification Enumerations */
typedef enum {
    BTN_EVENT_NONE = 0,
    BTN_EVENT_SHORT_PRESS,  /* 25 ms <= duration < 1000 ms */
    BTN_EVENT_LONG_PRESS    /* duration >= 1000 ms */
} ButtonPressEvent_t;

/* LED Operating Mode Enumerations */
typedef enum {
    LED_MODE_MANUAL_STATIC = 0,
    LED_MODE_CONTINUOUS_BLINK
} LedOperatingMode_t;

void SystemClock_Config(void);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
