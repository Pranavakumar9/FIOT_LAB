# Sub-Task 7A: 4-Bit HD44780 LCD Driver Initialization & Multi-Line Display

## 1. Title, CAT, Week, Date
- **Title:** 4-Bit HD44780 LCD Driver Initialization & Character/String Display
- **Evaluation:** CAT 7 (Sub-Task 7A)
- **Course:** 25CS2741 Fundamentals of Internet of Things + Lab
- **Week:** Week 6
- **Date:** 24/08/2026
- **Target Device:** Atmel AT89S52 @ 11.0592 MHz

---

## 2. Objective
1. Implement a 4-bit GPIO-based LCD driver for the Hitachi HD44780 controller by deriving the exact power-on command sequence from the manufacturer datasheet.
2. Configure the LCD in 4-bit bus mode, 2 display lines, and 5x8 dot matrix font format.
3. Develop the fundamental driver routines: `lcd_init()`, `lcd_cmd()`, `lcd_data()`, `lcd_puts()`, and `lcd_goto()`.
4. Render individual characters and full strings on both rows of the 16x2 display.
5. Strictly observe bare-metal register C restrictions without Arduino or third-party libraries.

---

## 3. Problem Statement (Paraphrased)
The student must construct an Embedded C driver for a 16x2 character LCD connected to the AT89S52 in 4-bit bus mode. The driver must correctly execute the initialization protocol specified in the HD44780 datasheet, taking into account power-on reset delays. The program must demonstrate writing instructions to the command register (`RS = 0`), sending ASCII characters to the data register (`RS = 1`), and displaying custom strings across both the first and second display lines.

---

## 4. Hardware and Software
- **Target Microcontroller:** Atmel AT89S52 (40-pin DIP)
- **Clock Source:** 11.0592 MHz Quartz Crystal with dual 33 pF ceramic capacitors
- **Display Module:** 16x2 Alphanumeric Character LCD (HD44780 / KS0066 compatible)
- **Contrast Trimmer:** 10 k$\Omega$ single-turn potentiometer
- **Compiler:** Keil C51 Compiler (v9.60a) / SDCC

---

## 5. Pin Connection Table

| AT89S52 Pin | Physical Pin # | Direction | Connected LCD Pin | Signal Description | Electrical Role |
|---|---|---|---|---|---|
| **P2.0** | Pin 21 | Output | Pin 4 (RS) | Register Select | 0 = Command, 1 = Data |
| **GND** | Pin 20 | Power | Pin 5 (RW) | Read / Write | Hardwired to 0V (Write mode) |
| **P2.2** | Pin 23 | Output | Pin 6 (EN) | Enable Strobe | Falling edge latches data |
| **P2.4** | Pin 25 | Output | Pin 11 (D4) | Data Bit 4 | High nibble bit 0 |
| **P2.5** | Pin 26 | Output | Pin 12 (D5) | Data Bit 5 | High nibble bit 1 |
| **P2.6** | Pin 27 | Output | Pin 13 (D6) | Data Bit 6 | High nibble bit 2 |
| **P2.7** | Pin 28 | Output | Pin 14 (D7) | Data Bit 7 | High nibble bit 3 |
| **NC** | — | — | Pins 7-10 (D0-D3)| Lower Data Bits | Unconnected in 4-bit mode |
| **V0** | Pin 3 of LCD | Input | 10k Pot Wiper | Contrast Voltage | Adjustable 0V to +5V |

---

## 6. Circuit Diagram and Explanation

![Sub-Task 7A Circuit Diagram](circuit/cat7_lcd_init.png)
*(Vector source available at [`circuit/cat7_lcd_init.svg`](circuit/cat7_lcd_init.svg))*

### Circuit Functionality
- **4-Bit Data Bus Wiring:** Data lines D4 through D7 of the LCD connect to the upper nibble of Port 2 (P2.4–P2.7). Control pins RS and EN connect to P2.0 and P2.2 respectively. The RW line (Pin 5) is permanently grounded because the driver performs write operations exclusively, eliminating bus turnaround delays.
- **Backlight Circuit:** Anode (Pin 15) connects to +5V through a 220 $\Omega$ current-limiting resistor, and Cathode (Pin 16) connects to GND, ensuring safe backlight LED current ($I \approx 15\text{ mA}$).

---

## 7. Theory and Datasheet Command Derivation

### 1. HD44780 4-Bit Power-On Reset Sequence
Upon power application, the internal reset circuit of the HD44780 requires $V_{DD}$ to rise above 4.5V for at least 15 ms. The microcontroller cannot rely on default register states and must execute the following hardware-mandated synchronization sequence:
1. Wait at least **15 ms** after $V_{DD} \ge 4.5\text{ V}$.
2. Write nibble `0x30` to data bus; pulse `EN`. Wait at least **4.1 ms**.
3. Write nibble `0x30` to data bus; pulse `EN`. Wait at least **100 µs**.
4. Write nibble `0x30` to data bus; pulse `EN`. (Completes 8-bit reset sync).
5. Write nibble `0x20` to data bus; pulse `EN`. (Switches interface to 4-bit bus mode!).

### 2. Operational Configuration Commands
Once in 4-bit mode, every command is transmitted as two consecutive 4-bit nibbles:
- **`0x28` (Function Set):** DL=0 (4-bit mode), N=1 (2 display lines), F=0 ($5 \times 8$ dots character font).
- **`0x0C` (Display ON/OFF Control):** D=1 (Display ON), C=0 (Cursor OFF), B=0 (Blink OFF).
- **`0x06` (Entry Mode Set):** I/D=1 (Increment cursor position rightward), S=0 (No display shift).
- **`0x01` (Clear Display):** Writes space character (`0x20`) to all 80 DDRAM locations and resets address counter to 0. Execution time: $1.64\text{ ms}$.

---

## 8. Code Walkthrough
Implementation in [`src/lcd_hd44780.c`](src/lcd_hd44780.c) and [`src/main.c`](src/main.c):
1. **Nibble Shifter and Enable Strobe:**
   ```c
   static void lcd_write_nibble(unsigned char nibble) {
       P2 = (P2 & 0x0F) | (nibble & 0xF0);
       PIN_LCD_EN = 1;
       delay_approx_us(5);
       PIN_LCD_EN = 0;
       delay_approx_us(50);
   }
   ```
2. **Byte Splitter (Command & Data):**
   `lcd_cmd()` sets `PIN_LCD_RS = 0`, transmits the high nibble (`cmd & 0xF0`), followed by the low nibble shifted left (`(cmd << 4) & 0xF0`). `lcd_data()` sets `PIN_LCD_RS = 1` and repeats the same nibble transmission.
3. **Application Superloop:**
   ```c
   lcd_init();
   lcd_goto(0, 0);
   lcd_data('A');
   lcd_goto(0, 2);
   lcd_puts("HD44780 4-BIT");
   lcd_goto(1, 0);
   lcd_puts("AT89S52 DRIVER");
   ```

---

## 9. Build, Flash and Run Steps
1. Add [`src/lcd_hd44780.c`](src/lcd_hd44780.c) and [`src/main.c`](src/main.c) to the Keil µVision target.
2. Build the project (`F7`):
   ```text
   compiling lcd_hd44780.c...
   compiling main.c...
   linking...
   Program Size: data=15.0 xdata=0 code=278
   creating hex file from "CAT7_TaskA_LCD_Init"...
   "CAT7_TaskA_LCD_Init" - 0 Error(s), 0 Warning(s).
   ```
3. Flash the AT89S52:
   ```bash
   avrdude -c usbasp -p 89s52 -U flash:w:CAT7_TaskA_LCD_Init.hex:i
   ```

---

## 10. Expected Output
The 16x2 character display initializes and shows:
```text
Row 0: A HD44780 4-BIT
Row 1: AT89S52 DRIVER 
```
- Character 'A' is located at coordinates (0, 0).
- High contrast, dark text against an illuminated green/blue backlight.

---

## 11. Viva Q&A (Technical Assessment)
1. **Q: Why does the HD44780 require sending `0x30` three times during power-on initialization?**  
   *A:* The power-on reset state of the HD44780 is uncertain if the power supply rise time ($V_{DD}$ ramp) does not meet datasheet specifications. Sending `0x30` three times forces the internal state machine into a known 8-bit mode regardless of whether it was previously in 4-bit or 8-bit mode, allowing a clean transition to 4-bit mode via `0x20`.
2. **Q: What is the purpose of the `EN` (Enable) pin?**  
   *A:* The `EN` pin serves as the data strobe clock. The HD44780 samples and latches data present on D4–D7 on the falling (high-to-low) edge of the `EN` pulse. The pulse must remain HIGH for at least $450\text{ ns}$ ($t_{PW}$) to guarantee valid capture.
3. **Q: What is the difference between CGROM, CGRAM, and DDRAM?**  
   *A:*
   - **DDRAM (Display Data RAM):** Stores character ASCII codes to be displayed on screen.
   - **CGROM (Character Generator ROM):** Read-only font memory containing standard $5 \times 8$ character dot matrix bitmaps.
   - **CGRAM (Character Generator RAM):** User-programmable RAM allowing up to 8 custom user-defined symbols.
4. **Q: Why must the Clear Display command (`0x01`) be followed by a longer delay ($\approx 2\text{ ms}$)?**  
   *A:* Most commands execute within $37\text{--}40\text{ }\mu\text{s}$. However, Clear Display (`0x01`) and Return Home (`0x02`) clear the entire 80-byte DDRAM and reset internal pointers, which requires up to $1.64\text{ ms}$ of internal processing time.

---

## 12. Troubleshooting
- **Top row shows a row of solid dark rectangular blocks:**
  - Indicates the LCD controller is receiving power but has failed initialization.
  - Check power supply ramp delay; increase startup delay to 30 ms.
  - Verify that `EN` pulses from HIGH to LOW.
- **Screen is completely blank:**
  - Adjust the 10 k$\Omega$ contrast potentiometer on Pin 3 (V0). If V0 is at +5V, characters are completely invisible.

---

## 13. Result and Learning Outcomes
- Successfully derived and implemented the HD44780 4-bit initialization protocol.
- Developed low-level driver primitives for command and character writing.
- Verified multi-line character and string display across both lines of a 16x2 LCD.
