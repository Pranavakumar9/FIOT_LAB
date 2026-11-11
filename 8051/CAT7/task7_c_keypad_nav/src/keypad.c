/**
 * ==============================================================================
 * @file           : keypad.c
 * @brief          : 4x4 Keypad Implementation for LCD Navigation
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT7 (Week 6 - 24/08/2026) - Task 7C
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#include "keypad.h"

#if defined(__GNUC__)
  static volatile unsigned char P1;
  #define ROM_CODE
#elif defined(__SDCC)
  #include <8052.h>
  #define ROM_CODE __code
#else
  #include <reg51.h>
  #define ROM_CODE code
#endif

static const unsigned char ROM_CODE KEY_MATRIX[4][4] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

void keypad_init(void)
{
    P1 = 0xFF; /* Float bus lines high */
}

unsigned char keypad_poll(void)
{
    unsigned char r;
    unsigned char c;
    unsigned char col_bits;

    for (r = 0; r < 4; r++)
    {
        /* Drive active row LOW */
        P1 = 0xFF & ~(1 << r);

        /* Read column sense lines (P1.4 - P1.7) */
        col_bits = (P1 >> 4) & 0x0F;

        if (col_bits != 0x0F)
        {
            for (c = 0; c < 4; c++)
            {
                if (!(col_bits & (1 << c)))
                {
                    P1 = 0xFF;
                    return KEY_MATRIX[r][c];
                }
            }
        }
    }

    P1 = 0xFF;
    return KEY_NONE;
}
