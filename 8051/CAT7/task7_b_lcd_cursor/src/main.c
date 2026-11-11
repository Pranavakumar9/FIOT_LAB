/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : Cursor Positioning and Dynamic Numeric Telemetry Updating
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT7 (Week 6 - 24/08/2026) - Task 7B
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Requirements:
 *   - Implement lcd_goto(row, column)
 *   - Implement lcd_puts(string)
 *   - Implement lcd_print_number(value)
 *   - Dynamically update real-time telemetry metrics on 16x2 screen
 * ==============================================================================
 */

#include "lcd_hd44780.h"

#if defined(__GNUC__)
  #define _nop_() __asm__ volatile ("nop")
#elif defined(__SDCC)
  #define _nop_() __asm nop __endasm
#else
  #include <intrins.h>
#endif

static void delay_ms(unsigned int ms)
{
    unsigned int i;
    unsigned char j;
    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 118; j++)
        {
            _nop_();
        }
    }
}

void main(void)
{
    unsigned int sample_counter = 0;
    unsigned int voltage_mv = 4980;

    lcd_init();

    /* Render static layout headers */
    lcd_goto(0, 0);
    lcd_puts("COUNT:");

    lcd_goto(1, 0);
    lcd_puts("SUPPLY:");

    while (1)
    {
        /* Dynamic value update at row 0, col 7 */
        lcd_goto(0, 7);
        lcd_print_number(sample_counter);
        lcd_puts(" cyc ");

        /* Dynamic value update at row 1, col 8 */
        lcd_goto(1, 8);
        lcd_print_number(voltage_mv);
        lcd_puts("mV");

        sample_counter++;
        if (sample_counter > 9999)
        {
            sample_counter = 0;
        }

        delay_ms(500); /* 500 ms display refresh rate */
    }
}
