/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : 16x2 LCD Initialization and Character/String Demonstration
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT7 (Week 6 - 24/08/2026) - Task 7A
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Requirements:
 *   - Configure LCD in 4-bit, 2-line mode.
 *   - Implement lcd_init(), lcd_cmd(), and lcd_data().
 *   - Display individual characters and strings.
 *   - Display text on both rows.
 * ==============================================================================
 */

#include "lcd_hd44780.h"

void main(void)
{
    /* Initialize HD44780 controller into 4-bit mode */
    lcd_init();

    /* Requirement 1: Display individual character 'A' at first position of row 1 */
    lcd_goto(0, 0);
    lcd_data('A');

    /* Requirement 2: Display string on first row */
    lcd_goto(0, 2);
    lcd_puts("HD44780 4-BIT");

    /* Requirement 3: Display string on second row */
    lcd_goto(1, 0);
    lcd_puts("AT89S52 DRIVER");

    while (1)
    {
        /* Static display hold */
    }
}
