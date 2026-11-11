/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : Interactive 4x4 Keypad-Based Character Navigation on 16x2 LCD
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT7 (Week 6 - 24/08/2026) - Task 7C
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Directional Control Keys:
 *   - Key 2: Move Cursor Up    (Row 1 -> Row 0)
 *   - Key 8: Move Cursor Down  (Row 0 -> Row 1)
 *   - Key 4: Move Cursor Left  (Column decrements with boundary check at 0)
 *   - Key 6: Move Cursor Right (Column increments with boundary check at 15)
 * ==============================================================================
 */

#include "lcd_hd44780.h"
#include "keypad.h"

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
    unsigned char key;
    unsigned char last_key = KEY_NONE;
    signed char cursor_row = 0;
    signed char cursor_col = 0;

    lcd_init();
    keypad_init();

    /* Render character at starting coordinates (0, 0) */
    lcd_goto(cursor_row, cursor_col);
    lcd_data('*');

    while (1)
    {
        key = keypad_poll();

        /* Process state change on new key press */
        if (key != last_key)
        {
            delay_ms(25); /* 25 ms debounce dwell */
            last_key = key;

            if (key != KEY_NONE)
            {
                unsigned char moved = 0;

                switch (key)
                {
                    case '2': /* UP */
                        if (cursor_row > 0)
                        {
                            cursor_row--;
                            moved = 1;
                        }
                        break;

                    case '8': /* DOWN */
                        if (cursor_row < 1)
                        {
                            cursor_row++;
                            moved = 1;
                        }
                        break;

                    case '4': /* LEFT */
                        if (cursor_col > 0)
                        {
                            cursor_col--;
                            moved = 1;
                        }
                        break;

                    case '6': /* RIGHT */
                        if (cursor_col < 15)
                        {
                            cursor_col++;
                            moved = 1;
                        }
                        break;

                    default:
                        break;
                }

                if (moved)
                {
                    lcd_clear();
                    lcd_goto(cursor_row, cursor_col);
                    lcd_data('*');
                }
            }
        }
    }
}
