/**
 * ==============================================================================
 * @file           : keypad.c
 * @brief          : 4x4 Matrix Keypad Scanning Driver Implementation
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT4 (Week 3 - 03/08/2026)
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

/* Keypad Character Matrix Layout */
static const unsigned char ROM_CODE KEYPAD_LUT[4][4] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

void keypad_init(void)
{
    /* Float all lines high: Rows output 1, Columns ready to sense */
    P1 = 0xFF;
}

unsigned char keypad_get_key(void)
{
    unsigned char row_idx;
    unsigned char col_idx;
    unsigned char pin_state;
    unsigned char detected_char = NO_KEY_PRESSED;

    for (row_idx = 0; row_idx < 4; row_idx++)
    {
        /* Drive current row LOW while keeping other rows HIGH */
        P1 = 0xFF & ~(1 << row_idx);

        /* Read column sense lines (P1.4 - P1.7) */
        pin_state = (P1 >> 4) & 0x0F;

        if (pin_state != 0x0F)
        {
            /* Check which column line is pulled LOW */
            for (col_idx = 0; col_idx < 4; col_idx++)
            {
                if (!(pin_state & (1 << col_idx)))
                {
                    detected_char = KEYPAD_LUT[row_idx][col_idx];
                    P1 = 0xFF; /* Restore bus */
                    return detected_char;
                }
            }
        }
    }

    P1 = 0xFF; /* Float bus high */
    return NO_KEY_PRESSED;
}
