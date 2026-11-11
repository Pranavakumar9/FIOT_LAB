/**
 * ==============================================================================
 * @file           : lcd_hd44780.h
 * @brief          : 16x2 HD44780 LCD Driver Header (4-Bit Bus Mode)
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT7 (Week 6 - 24/08/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#ifndef LCD_HD44780_H
#define LCD_HD44780_H

/**
 * @brief  Derives 4-bit initialization sequence from HD44780 datasheet and
 *         configures display in 4-bit, 2-line, 5x8 dot matrix font.
 */
void lcd_init(void);

/**
 * @brief  Transmits an instruction byte to the LCD command register (RS = 0).
 * @param  cmd: Command code (e.g., 0x01 = Clear, 0x02 = Home).
 */
void lcd_cmd(unsigned char cmd);

/**
 * @brief  Transmits an ASCII character to LCD data register (RS = 1).
 * @param  data_byte: ASCII character to render at cursor position.
 */
void lcd_data(unsigned char data_byte);

/**
 * @brief  Sets DDRAM address to position cursor at specific row and column.
 * @param  row: Line number (0 = Line 1, 1 = Line 2).
 * @param  col: Column offset (0 to 15).
 */
void lcd_goto(unsigned char row, unsigned char col);

/**
 * @brief  Renders a null-terminated string starting at current cursor position.
 * @param  str: Pointer to string in ROM or RAM.
 */
void lcd_puts(const char *str);

/**
 * @brief  Renders an unsigned integer as decimal digits.
 * @param  value: Unsigned 16-bit integer (0 to 65535).
 */
void lcd_print_number(unsigned int value);

/**
 * @brief  Clears LCD display screen and returns cursor to Home (0, 0).
 */
void lcd_clear(void);

#endif /* LCD_HD44780_H */
