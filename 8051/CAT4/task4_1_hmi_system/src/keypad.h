/**
 * ==============================================================================
 * @file           : keypad.h
 * @brief          : 4x4 Matrix Keypad Driver Header
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT4 (Week 3 - 03/08/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#ifndef KEYPAD_H
#define KEYPAD_H

#define NO_KEY_PRESSED 0xFF

/**
 * @brief  Initializes the matrix keypad GPIO port lines.
 */
void keypad_init(void);

/**
 * @brief  Scans the 4x4 matrix keypad and returns the ASCII value of a pressed key.
 * @return ASCII character ('0'-'9', 'A'-'D', '*', '#') or NO_KEY_PRESSED if idle.
 */
unsigned char keypad_get_key(void);

#endif /* KEYPAD_H */
