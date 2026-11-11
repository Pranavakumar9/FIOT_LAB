# Sub-Task 7B: LCD Cursor Positioning & Dynamic Decimal Value Updating

## 1. Title, CAT, Week, Date
- **Title:** LCD Cursor Positioning & Dynamic Telemetry Numeric Formatting
- **Evaluation:** CAT 7 (Sub-Task 7B)
- **Course:** 25CS2741 Fundamentals of Internet of Things + Lab
- **Week:** Week 6
- **Date:** 24/08/2026
- **Target Device:** Atmel AT89S52 @ 11.0592 MHz

---

## 2. Objective
1. Implement absolute cursor positioning routines (`lcd_goto(row, column)`) based on HD44780 DDRAM address mapping.
2. Develop a binary-to-decimal string formatter (`lcd_print_number(unsigned int value)`) without relying on bulky standard C library functions (`sprintf`).
3. Render formatted static interface templates and dynamically update changing telemetry metrics (cycle counts, rail voltages) on the 16x2 screen.
4. Eliminate display flickering during continuous numeric refreshes.

---

## 3. Problem Statement (Paraphrased)
The student must extend the 16x2 LCD driver to support dynamic cursor positioning and numeric value rendering. The program must implement `lcd_goto(row, column)`, `lcd_puts(string)`, and `lcd_print_number(value)`. The firmware must display static label headers on the screen and periodically update real-time sensor/telemetry counter values at specified coordinates without clearing the whole screen or causing visual tearing.

---

## 4. Hardware and Software
- **Target Microcontroller:** Atmel AT89S52 (40-pin DIP)
- **Clock Source:** 11.0592 MHz Quartz Crystal with dual 33 pF ceramic capacitors
- **Display Module:** 16x2 Alphanumeric Character LCD in 4-bit bus mode on Port 2
- **Instrumentation:** Digital Multimeter
- **Compiler:** Keil C51 Compiler (v9.60a) / SDCC

---

## 5. Pin Connection Table

| AT89S52 Pin | Physical Pin # | Direction | Connected LCD Pin | Signal Description | Role |
|---|---|---|---|---|---|
| **P2.0** | Pin 21 | Output | Pin 4 (RS) | Register Select | Low for DDRAM address, High for data |
| **GND** | Pin 20 | Power | Pin 5 (RW) | Read/Write | Grounded (Write mode) |
| **P2.2** | Pin 23 | Output | Pin 6 (EN) | Enable Strobe | Latches high/low nibble on falling edge |
| **P2.4 - P2.7** | Pins 25 - 28 | Output | Pins 11 - 14 (D4 - D7)| High Data Nibble | 4-bit multiplexed bus |
| **V0** | LCD Pin 3 | Analog | 10k Potentiometer Wiper | Contrast Adjust | Sets liquid crystal bias voltage |

---

## 6. Circuit Diagram and Explanation

![Sub-Task 7B Circuit Diagram](circuit/cat7_lcd_cursor.png)
*(Vector source available at [`circuit/cat7_lcd_cursor.svg`](circuit/cat7_lcd_cursor.svg))*

### Circuit Functionality
The hardware configuration uses the 4-bit Port 2 topology. Control lines RS and EN latch address commands and character data. RW is grounded for continuous write capability.

---

## 7. Theory and DDRAM Addressing Mathematics

### 1. HD44780 DDRAM Architecture
The internal Display Data RAM of the HD44780 has 80 bytes of storage, organized into two rows:
- **Line 1:** 40 bytes (Addresses `0x00` through `0x27`). The first 16 bytes (`0x00` to `0x0F`) are visible on screen.
- **Line 2:** 40 bytes (Addresses `0x40` through `0x67`). The first 16 bytes (`0x40` to `0x4F`) are visible on screen.

To set the cursor location, command `0x80` (Set DDRAM Address) is combined with the line base address and column offset:
$$\text{Line 1 Command} = 0\text{x80} + \text{column} \quad (\text{Range: } 0\text{x80 to } 0\text{x8F})$$
$$\text{Line 2 Command} = 0\text{x80} + 0\text{x40} + \text{column} = 0\text{xC0} + \text{column} \quad (\text{Range: } 0\text{xC0 to } 0\text{xCF})$$

### 2. Lightweight Integer Conversion
In resource-constrained 8051 systems with only 256 bytes of RAM, calling `sprintf()` links floating-point formatting libraries that consume over 2.5 KB of code space.
`lcd_print_number()` implements an efficient modulo-10 algorithm:
1. Successively divide integer by 10 (`val % 10`), extracting ASCII digits into a local 5-byte array in reverse order.
2. Emit digits in forward order to the LCD data register.

---

## 8. Code Walkthrough
Implementation in [`src/lcd_hd44780.c`](src/lcd_hd44780.c) and [`src/main.c`](src/main.c):
1. **Cursor Positioning Routine:**
   ```c
   void lcd_goto(unsigned char row, unsigned char col) {
       unsigned char addr = (row == 0) ? (0x80 + (col & 0x0F)) : (0xC0 + (col & 0x0F));
       lcd_cmd(addr);
   }
   ```
2. **Numeric Print Algorithm:**
   ```c
   void lcd_print_number(unsigned int value) {
       char digits[5];
       signed char idx = 0;
       if (value == 0) { lcd_data('0'); return; }
       while (value > 0) {
           digits[idx++] = (value % 10) + '0';
           value /= 10;
       }
       while (--idx >= 0) {
           lcd_data((unsigned char)digits[idx]);
       }
   }
   ```
3. **Flicker-Free Refresh Technique:** The application avoids calling `lcd_clear()` in the loop. Instead, `lcd_goto(row, col)` moves the cursor directly to the field location, writes the updated numbers, and adds trailing space characters to overwrite residual digits.

---

## 9. Build, Flash and Run Steps
1. In Keil µVision, compile and link [`src/lcd_hd44780.c`](src/lcd_hd44780.c) and [`src/main.c`](src/main.c):
   ```text
   compiling lcd_hd44780.c...
   compiling main.c...
   linking...
   Program Size: data=19.0 xdata=0 code=342
   creating hex file from "CAT7_TaskB_LCD_Cursor"...
   "CAT7_TaskB_LCD_Cursor" - 0 Error(s), 0 Warning(s).
   ```
2. Flash the target MCU:
   ```bash
   avrdude -c usbasp -p 89s52 -U flash:w:CAT7_TaskB_LCD_Cursor.hex:i
   ```

---

## 10. Expected Output
The 16x2 LCD screen displays:
```text
Row 0: COUNT:  142 cyc 
Row 1: SUPPLY: 4980mV  
```
- The cycle counter on Row 0 increments dynamically every 500 ms (`0, 1, 2, ... 9999`).
- Supply voltage displays static calibrated rail reading `4980mV`.
- Screen updates smoothly without blinking or display flickering.

---

## 11. Viva Q&A (Technical Assessment)
1. **Q: Why does Line 2 of a 16x2 LCD start at DDRAM address `0x40` instead of `0x10`?**  
   *A:* The HD44780 controller is designed to support displays up to 40 characters wide. The internal DDRAM is partitioned into two 40-byte buffers: Line 1 starts at address `0x00` (`0x00` to `0x27`), and Line 2 starts at address `0x40` (`0x40` to `0x67`). On a 16x2 screen, columns 16 through 39 exist in memory but are physically off-screen.
2. **Q: Why causes screen flickering when updating numbers on an LCD?**  
   *A:* Clearing the display (`0x01`) inside a loop before writing new data causes noticeable flashing because the screen turns entirely blank for ~2 ms before characters are redrawn. Repositioning the cursor via `lcd_goto()` and overwriting only the changing numeric field eliminates flickering.
3. **Q: What is the maximum value that can be rendered by `lcd_print_number(unsigned int value)` on an 8051?**  
   *A:* In Keil C51, an `unsigned int` is a 16-bit integer, accommodating values from `0` to `65535` (up to 5 decimal digits).
4. **Q: How do trailing space characters prevent display artifacts when a number decreases in magnitude?**  
   *A:* When a number transitions from 4 digits (e.g. `1000`) to 3 digits (e.g. `999`), failing to overwrite the 4th position leaves the previous character visible, causing the display to erroneously read `9990`. Emitting trailing spaces clears leftover characters.

---

## 12. Troubleshooting
- **Characters appear in the middle of Line 1 instead of Line 2:**
  - Verify that the base address for Line 2 is `0xC0` (`0x80 + 0x40`) and not `0x90`.
- **Numbers print in reverse order (e.g. `123` prints as `321`):**
  - Verify that digits extracted via modulo-10 are pushed into an array and printed in Last-In-First-Out (LIFO) order.

---

## 13. Result and Learning Outcomes
- Successfully implemented absolute DDRAM cursor positioning on a 16x2 LCD.
- Engineered a lightweight binary-to-decimal numeric rendering routine.
- Verified dynamic telemetry updates without visual display flickering.
