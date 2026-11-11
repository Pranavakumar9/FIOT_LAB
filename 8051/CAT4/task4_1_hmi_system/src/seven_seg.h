/**
 * ==============================================================================
 * @file           : seven_seg.h
 * @brief          : Seven-Segment Display Driver Header
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT4 (Week 3 - 03/08/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#ifndef SEVEN_SEG_H
#define SEVEN_SEG_H

/**
 * @brief  Initializes the 7-segment display port.
 */
void seven_seg_init(void);

/**
 * @brief  Renders a numeric decimal digit (0 through 9) on the 7-segment display.
 * @param  value: Binary digit (0 to 9).
 */
void display_digit(unsigned char value);

/**
 * @brief  Clears/extinguishes the 7-segment display.
 */
void display_clear(void);

#endif /* SEVEN_SEG_H */
