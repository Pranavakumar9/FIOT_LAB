/**
 * ==============================================================================
 * @file           : bsp_delay.h
 * @brief          : Board Support Package Software Delay Library Header
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT2 (Week 2 - 20/07/2026)
 * @target         : Atmel AT89S52
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#ifndef BSP_DELAY_H
#define BSP_DELAY_H

/**
 * @brief  Provides busy-wait blocking delay in milliseconds.
 * @param  ms: Number of milliseconds (1 to 65535).
 * @note   Calibrated for 11.0592 MHz 12-clock 8051 core.
 */
void bsp_delay_ms(unsigned int ms);

/**
 * @brief  Provides fine-grain busy-wait blocking delay in microseconds.
 * @param  us: Number of microseconds (~10 to 65535 us).
 */
void bsp_delay_us(unsigned int us);

#endif /* BSP_DELAY_H */
