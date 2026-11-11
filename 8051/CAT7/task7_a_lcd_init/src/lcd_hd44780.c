/**
 * ==============================================================================
 * @file           : lcd_hd44780.c
 * @brief          : 16x2 HD44780 LCD Driver Implementation (4-Bit Bus Mode)
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT7 (Week 6 - 24/08/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#include "lcd_hd44780.h"

#if defined(__GNUC__)
  static volatile unsigned char P2;
  static volatile unsigned char PIN_LCD_RS;
  static volatile unsigned char PIN_LCD_EN;
  #define _nop_() __asm__ volatile ("nop")
#elif defined(__SDCC)
  #include <8052.h>
  #define PIN_LCD_RS P2_0
  #define PIN_LCD_EN P2_2
  #define _nop_() __asm nop __endasm
#else
  #include <reg51.h>
  #include <intrins.h>
  sbit PIN_LCD_RS = P2^0;
  sbit PIN_LCD_EN = P2^2;
#endif

/* Calibrated microsecond and millisecond busy-wait helpers */
static void delay_approx_us(unsigned int us)
{
    while (us--)
    {
        _nop_();
    }
}

static void delay_approx_ms(unsigned int ms)
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

/**
 * @brief  Generates a high-to-low latch strobe pulse on the LCD Enable (EN) pin.
 */
static void lcd_strobe_enable(void)
{
    PIN_LCD_EN = 1;
    delay_approx_us(5);
    PIN_LCD_EN = 0;
    delay_approx_us(50);
}

/**
 * @brief  Writes a 4-bit nibble to Port 2 high nibble (P2.4 - P2.7).
 * @param  nibble: 4-bit value aligned in bits 4-7.
 */
static void lcd_write_nibble(unsigned char nibble)
{
    P2 = (P2 & 0x0F) | (nibble & 0xF0);
    lcd_strobe_enable();
}

void lcd_cmd(unsigned char cmd)
{
    PIN_LCD_RS = 0; /* Command Register Select */

    /* Transmit high nibble followed by low nibble */
    lcd_write_nibble(cmd & 0xF0);
    lcd_write_nibble((cmd << 4) & 0xF0);

    /* Command execution dwell */
    if (cmd == 0x01 || cmd == 0x02)
    {
        delay_approx_ms(2); /* Clear and Home require > 1.64 ms */
    }
    else
    {
        delay_approx_us(50);
    }
}

void lcd_data(unsigned char data_byte)
{
    PIN_LCD_RS = 1; /* Data Register Select */

    /* Transmit high nibble followed by low nibble */
    lcd_write_nibble(data_byte & 0xF0);
    lcd_write_nibble((data_byte << 4) & 0xF0);

    delay_approx_us(50);
}

void lcd_init(void)
{
    PIN_LCD_RS = 0;
    PIN_LCD_EN = 0;

    /* Step 1: Wait > 15 ms after power-on (VCC reaches 4.5V) */
    delay_approx_ms(30);

    /* Step 2: Special 8-bit command sequence to force reset into 4-bit mode */
    lcd_write_nibble(0x30);
    delay_approx_ms(5);   /* Wait > 4.1 ms */

    lcd_write_nibble(0x30);
    delay_approx_ms(1);   /* Wait > 100 us */

    lcd_write_nibble(0x30);
    delay_approx_ms(1);

    /* Step 3: Switch interface to 4-bit mode */
    lcd_write_nibble(0x20);
    delay_approx_ms(1);

    /* Step 4: Configure Function Set: 4-bit mode, 2 display lines, 5x8 font */
    lcd_cmd(0x28);

    /* Step 5: Display Control: Display ON, Cursor OFF, Blink OFF */
    lcd_cmd(0x0C);

    /* Step 6: Entry Mode Set: Auto-increment cursor right, no display shift */
    lcd_cmd(0x06);

    /* Step 7: Clear entire DDRAM and return home */
    lcd_clear();
}

void lcd_goto(unsigned char row, unsigned char col)
{
    /*
     * HD44780 DDRAM Memory Map:
     * Line 0: Base address 0x80 (Columns 0 to 15 -> 0x80 to 0x8F)
     * Line 1: Base address 0xC0 (Columns 0 to 15 -> 0xC0 to 0xCF)
     */
    unsigned char ddram_addr;

    if (row == 0)
    {
        ddram_addr = 0x80 + (col & 0x0F);
    }
    else
    {
        ddram_addr = 0xC0 + (col & 0x0F);
    }

    lcd_cmd(ddram_addr);
}

void lcd_puts(const char *str)
{
    while (*str)
    {
        lcd_data((unsigned char)*str);
        str++;
    }
}

void lcd_print_number(unsigned int value)
{
    char digits[5];
    signed char idx = 0;

    if (value == 0)
    {
        lcd_data('0');
        return;
    }

    /* Extract base-10 digits in reverse order */
    while (value > 0)
    {
        digits[idx++] = (value % 10) + '0';
        value /= 10;
    }

    /* Print digits in correct forward order */
    while (--idx >= 0)
    {
        lcd_data((unsigned char)digits[idx]);
    }
}

void lcd_clear(void)
{
    lcd_cmd(0x01); /* Clear display command */
    delay_approx_ms(2);
}
