/**
 * ==============================================================================
 * @file           : keypad.h
 * @brief          : 4x4 Keypad Header for LCD Navigation
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT7 (Week 6 - 24/08/2026) - Task 7C
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#ifndef KEYPAD_H
#define KEYPAD_H

#define KEY_NONE 0xFF

void keypad_init(void);
unsigned char keypad_poll(void);

#endif /* KEYPAD_H */
