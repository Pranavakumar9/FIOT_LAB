/**
 * ==============================================================================
 * @file           : led_control.h
 * @brief          : LED Subsystem Control Routines Header
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT4 (Week 3 - 03/08/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

/**
 * @brief  Initializes the LED bus port lines (P2.0 - P2.3).
 */
void led_subsystem_init(void);

/**
 * @brief  Executes corresponding LED action associated with keypad ASCII character.
 * @param  key: ASCII character ('0' to '9').
 */
void led_control(unsigned char key);

/**
 * @brief  Periodic driver routines for dynamic animated modes
 */
void running_led(void);
void binary_counter(void);
void alternate_led(void);
void blink_all(void);

/**
 * @brief  Evaluates dynamic pattern timers in the main superloop.
 * @param  current_ms: Current system timebase tick count.
 */
void led_service_animation(unsigned int current_ms);

#endif /* LED_CONTROL_H */
