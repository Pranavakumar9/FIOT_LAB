# Sub-Task 7C: 4x4 Keypad-Based Character Navigation on 16x2 LCD

## 1. Title, CAT, Week, Date
- **Title:** Interactive 4x4 Matrix Keypad Character Navigation on 16x2 LCD
- **Evaluation:** CAT 7 (Sub-Task 7C)
- **Course:** 25CS2741 Fundamentals of Internet of Things + Lab
- **Week:** Week 6
- **Date:** 24/08/2026
- **Target Device:** Atmel AT89S52 @ 11.0592 MHz

---

## 2. Objective
1. Construct an interactive human-machine interface integrating a 4x4 matrix keypad on Port 1 with a 16x2 HD44780 character LCD on Port 2.
2. Program directional key controls:
   - **Key 2:** Move character UP (Row 1 $\to$ Row 0)
   - **Key 8:** Move character DOWN (Row 0 $\to$ Row 1)
   - **Key 4:** Move character LEFT (Column decrements, minimum 0)
   - **Key 6:** Move character RIGHT (Column increments, maximum 15)
3. Enforce boundary safety clamping to prevent cursor overrun beyond display limits (Row $\in [0, 1]$, Column $\in [0, 15]$).
4. Update display coordinates in real time upon valid debounced key events.

---

## 3. Problem Statement (Paraphrased)
The student must develop an interactive embedded LCD application in which a designated character (e.g. `'*'`) is dynamically moved across the 16x2 display grid using a 4x4 matrix keypad. Four keys must be configured as directional controllers: Key 2 for UP, Key 8 for DOWN, Key 4 for LEFT, and Key 6 for RIGHT. The firmware must ensure the character cannot be moved beyond the 2-row by 16-column boundaries, clearing previous positions and re-rendering the character at the new coordinates on each valid key press.

---

## 4. Hardware and Software
- **Target MCU:** Atmel AT89S52 (40-pin DIP)
- **Clock Source:** 11.0592 MHz Quartz Crystal with dual 33 pF ceramic capacitors
- **Display Module:** 16x2 HD44780 Character LCD (4-bit mode on Port 2)
- **Input Peripheral:** 4x4 Matrix Membrane / Tactile Keypad on Port 1
- **Compiler:** Keil C51 Compiler (v9.60a) / SDCC

---

## 5. Pin Connection Table

| Subsystem | Signal Name | AT89S52 Pin | Physical Pin # | Direction | Functional Role |
|---|---|---|---|---|---|
| **LCD** | RS | P2.0 | Pin 21 | Output | Command / Data Register Select |
| **LCD** | EN | P2.2 | Pin 23 | Output | High-to-Low Strobe Clock |
| **LCD** | D4 - D7 | P2.4 - P2.7 | Pins 25 - 28 | Output | 4-bit Multiplexed Data Bus |
| **LCD** | RW | Ground (GND)| Pin 20 | Power | Grounded (Write Mode) |
| **Keypad** | Rows R1 - R4 | P1.0 - P1.3 | Pins 1 - 4 | Output | Active-LOW Row Strobes |
| **Keypad** | Cols C1 - C4 | P1.4 - P1.7 | Pins 5 - 8 | Input | Column Senses (Pulled High Internally)|

---

## 6. Circuit Diagram and Explanation

![Sub-Task 7C Circuit Diagram](circuit/cat7_keypad_nav.png)
*(Vector source available at [`circuit/cat7_keypad_nav.svg`](circuit/cat7_keypad_nav.svg))*

### Circuit Functionality
- **Dual Peripheral Interfacing:** Port 1 interfaces the 16-switch matrix keypad, driving rows LOW and sampling column lines. Port 2 interfaces the 16x2 LCD in 4-bit mode. The AT89S52 coordinates both peripherals simultaneously without bus contention.

---

## 7. Theory and Coordinate State Logic

### 1. Matrix Keypad Directional Mapping
```
      [1]      [2:▲]     [3]      [A]   <-- Row 0 (P1.0)
      [4:◄]    [5]       [6:►]    [B]   <-- Row 1 (P1.1)
      [7]      [8:▼]     [9]      [C]   <-- Row 2 (P1.2)
      [*]      [0]       [#]      [D]   <-- Row 3 (P1.3)
       ▲        ▲         ▲        ▲
     Col 0    Col 1     Col 2    Col 3
    (P1.4)   (P1.5)    (P1.6)   (P1.7)
```

### 2. Boundary Clamping State Equations
Let current coordinates be $(r, c)$ where $r \in \{0, 1\}$ and $c \in \{0, 1, \dots, 15\}$:
- **Move UP (Key 2):**
  $$r' = \max(0, r - 1)$$
- **Move DOWN (Key 8):**
  $$r' = \min(1, r + 1)$$
- **Move LEFT (Key 4):**
  $$c' = \max(0, c - 1)$$
- **Move RIGHT (Key 6):**
  $$c' = \min(15, c + 1)$$

---

## 8. Code Walkthrough
Implementation in [`src/main.c`](src/main.c):
1. **Coordinate Initialization:**
   ```c
   signed char cursor_row = 0;
   signed char cursor_col = 0;
   lcd_goto(cursor_row, cursor_col);
   lcd_data('*');
   ```
2. **Keypad Interrogation & Debounce:** Samples `keypad_poll()`. Upon state transition, imposes a 25 ms debounce dwell.
3. **Movement Execution:**
   ```c
   switch (key) {
       case '2': if (cursor_row > 0) { cursor_row--; moved = 1; } break;
       case '8': if (cursor_row < 1) { cursor_row++; moved = 1; } break;
       case '4': if (cursor_col > 0) { cursor_col--; moved = 1; } break;
       case '6': if (cursor_col < 15) { cursor_col++; moved = 1; } break;
   }
   if (moved) {
       lcd_clear();
       lcd_goto(cursor_row, cursor_col);
       lcd_data('*');
   }
   ```

---

## 9. Build, Flash and Run Steps
1. Add [`src/lcd_hd44780.c`](src/lcd_hd44780.c), [`src/keypad.c`](src/keypad.c), and [`src/main.c`](src/main.c) to Keil µVision.
2. Build the project:
   ```text
   compiling lcd_hd44780.c...
   compiling keypad.c...
   compiling main.c...
   linking...
   Program Size: data=23.0 xdata=0 code=394
   creating hex file from "CAT7_TaskC_Keypad_Nav"...
   "CAT7_TaskC_Keypad_Nav" - 0 Error(s), 0 Warning(s).
   ```
3. Flash the AT89S52:
   ```bash
   avrdude -c usbasp -p 89s52 -U flash:w:CAT7_TaskC_Keypad_Nav.hex:i
   ```

---

## 10. Expected Output
- **Start:** Character `'*'` appears at Row 0, Column 0 (top-left corner).
- **Press '6':** Character moves right to $(0, 1), (0, 2), \dots, (0, 15)$.
- **Press '8':** Character drops down to Row 1 at coordinates $(1, 15)$.
- **Press '4':** Character moves left along Row 1 toward $(1, 0)$.
- **Press '2':** Character jumps up to Row 0.
- **Edge Boundary Test:** Pressing '2' while on Row 0 or '4' while on Column 0 clamps the position safely without screen wrapping or cursor loss.

---

## 11. Viva Q&A (Technical Assessment)
1. **Q: How does the firmware prevent screen wrapping when moving beyond Column 15?**  
   *A:* The firmware performs software boundary clamping: before incrementing column coordinate `c`, it checks `if (cursor_col < 15)`. If the character is already at column 15, the increment is ignored, preventing the cursor from advancing into the off-screen DDRAM memory range (`0x10` to `0x27`).
2. **Q: Why does the LCD need to be cleared when the character moves?**  
   *A:* If the display were not cleared (or the previous position overwritten with a space character), the character at the old coordinate would remain illuminated, leaving a trail of duplicate characters across the screen.
3. **Q: How can character movement speed be regulated to avoid uncontrolled multiple steps per press?**  
   *A:* By implementing stateful key detection where a movement event is triggered only on a new key transition (`key != last_key`) combined with a 25 ms debounce dwell, rather than continuously moving while a key is held down.
4. **Q: What is the benefit of defining cursor coordinates as `signed char` instead of `unsigned char`?**  
   *A:* Using a signed type prevents unsigned underflow issues where decrementing `0` would wrap around to `255`, which could defeat bounds checking if evaluated improperly.

---

## 12. Troubleshooting
- **Character wraps into hidden memory after Column 15:**
  - Verify that the column boundary check is strictly `< 15` and not `< 40`.
- **Character jumps two positions per single key press:**
  - Increase the debounce dwell to 35 ms.
  - Verify that the key transition logic waits for key release or compares `key != last_key`.

---

## 13. Result and Learning Outcomes
- Successfully integrated a 4x4 matrix keypad with a 16x2 HD44780 LCD driver.
- Implemented 4-directional character navigation with mathematical boundary safety clamping.
- Validated real-time interactive embedded HMI responsiveness.
