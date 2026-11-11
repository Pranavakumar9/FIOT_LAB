# Sub-Task 6B: Interrupt-Driven UART Driver with Circular Ring Buffer Echo

## 1. Title, CAT, Week, Date
- **Title:** Interrupt-Driven UART Driver & Circular FIFO Ring Buffer Character Echo
- **Evaluation:** CAT 6 (Sub-Task 6B)
- **Course:** 25CS2741 Fundamentals of Internet of Things + Lab
- **Week:** Week 5
- **Date:** 17/08/2026
- **Target Device:** Atmel AT89S52 @ 11.0592 MHz

---

## 2. Objective
1. Design and implement an interrupt-driven UART driver on the AT89S52 using Serial Interrupt Vector 4 (address `0x0023`).
2. Construct a lock-free circular FIFO ring buffer with power-of-2 depth (32 bytes) utilizing fast bitwise wrap-around masking.
3. Decouple high-speed asynchronous byte reception from application-level processing to prevent data loss and buffer overruns.
4. Echo incoming characters typed in PuTTY back to the PC terminal emulator interactively in real time.
5. Strictly observe bare-metal register C restrictions without using vendor or third-party libraries.

---

## 3. Problem Statement (Paraphrased)
The student must make the serial communication interactive. Whatever character is typed in a PC terminal (such as PuTTY) must be captured and returned by the 8051. Because user typing and host transmission occur asynchronously, polling registers can miss incoming characters. An interrupt-driven UART driver must be implemented, capturing bytes upon the setting of the RI flag and placing them into a circular RX ring buffer. The main loop must extract characters from the buffer and transmit them back to the terminal.

---

## 4. Hardware and Software
- **Microcontroller:** Atmel AT89S52 (40-pin DIP)
- **Oscillator:** 11.0592 MHz Quartz Crystal with dual 33 pF ceramic capacitors
- **Serial Interface:** USB-to-TTL Serial Bridge (CP2102 / CH340G / FT232R)
- **Terminal Emulator:** PuTTY configured for 9600-8-N-1 (Local Echo OFF)
- **Compiler:** Keil C51 Compiler (v9.60a) / SDCC

---

## 5. Pin Connection Table

| AT89S52 Pin | Physical Pin # | Direction | Signal Name | USB Bridge Terminal | Electrical Topology |
|---|---|---|---|---|---|
| **P3.0 (RXD)** | Pin 10 | Input | Serial Receive | USB Bridge **TXD** | 5.0 V TTL Cross-Wired |
| **P3.1 (TXD)** | Pin 11 | Output | Serial Transmit | USB Bridge **RXD** | 5.0 V TTL Cross-Wired |
| **GND** | Pin 20 | Power | Ground Reference | USB Bridge **GND** | 0.0 V Common Reference |
| **VCC** | Pin 40 | Power | Supply Voltage | Development Board VCC | +5.0 V DC |

---

## 6. Circuit Diagram and Explanation

![Sub-Task 6B Circuit Diagram](circuit/cat6_uart_echo.png)
*(Vector source available at [`circuit/cat6_uart_echo.svg`](circuit/cat6_uart_echo.svg))*

### Electrical Operation
The serial link operates as a full-duplex asynchronous channel. When the user presses a key on the workstation keyboard, PuTTY serializes the ASCII byte and transmits it over USB to the bridge IC. The bridge emits the bitstream on its TXD pin into AT89S52 P3.0 (RXD). Upon receiving the full stop bit, the 8051 hardware triggers the Vector 4 interrupt. The MCU processes the byte and shifts it back out on P3.1 (TXD) to the bridge RXD pin, returning the character to the PuTTY screen.

---

## 7. Theory and Ring Buffer Architecture

### 1. Vector 4 Interrupt Mechanics
In the 8051 MCS-51 core, both transmission (`TI`) and reception (`RI`) share a single interrupt vector:
- **Vector Address:** `0x0023` (Interrupt 4).
- **Flag Clearing:** Unlike timer interrupts where hardware clears `TF0` upon entering the ISR, the 8051 hardware **does not automatically clear `RI` or `TI`**. Software must explicitly clear the flag that triggered the interrupt:
  ```c
  if (RI) {
      char c = SBUF;
      RI = 0; /* Mandatory software clear */
      ring_buffer_put(&rx_fifo, c);
  }
  if (TI) {
      TI = 0; /* Mandatory software clear */
      tx_busy = 0;
  }
  ```

### 2. Power-of-2 Ring Buffer Optimization
Modulo arithmetic (`% BUFFER_SIZE`) on an 8-bit microcontroller without a hardware integer divider involves expensive software division routines consuming dozens of machine cycles.
By enforcing a buffer size that is an exact power of 2 ($N = 32 = 2^5$):
$$\text{Index Wrap} = (\text{index} + 1) \ \& \ (\text{SIZE} - 1) = (\text{index} + 1) \ \& \ 0\text{x1F}$$
This compiles into a single, high-speed single-cycle `ANL` (Logical AND) machine instruction.

```
       Head Pointer (ISR Enqueues)
               │
               ▼
  ┌───┬───┬───┬───┬───┬───┬───┬───┬───┐
  │ 'A' │ 'B' │ 'C' │ '1' │ '2' │ '3' │   │   │   │  ... (32 Bytes Storage)
  └───┴───┴───┴───┴───┴───┴───┴───┴───┘
               ▲
               │
        Tail Pointer (Main Superloop Dequeues & Echoes)
```

---

## 8. Code Walkthrough

### Ring Buffer Module: [`src/ring_buffer.h`](src/ring_buffer.h) & [`src/ring_buffer.c`](src/ring_buffer.c)
- `ring_buffer_put()`: Checks `next_head == tail_idx` to prevent buffer overflow. Stores incoming byte and updates `head_idx`.
- `ring_buffer_get()`: Checks `head_idx == tail_idx` for empty buffer condition. Extracts byte and increments `tail_idx`.

### Main Application: [`src/main.c`](src/main.c)
1. **Interrupt Configuration:**
   ```c
   ES = 1; /* Enable Serial Interrupt */
   EA = 1; /* Enable Global Interrupts */
   ```
2. **Interactive Echo Superloop:**
   ```c
   while (1) {
       if (ring_buffer_get(&rx_fifo, &echo_char)) {
           uart_send_byte(echo_char);
           if (echo_char == '\r') {
               uart_send_byte('\n'); /* CRLF normalization */
           }
       }
   }
   ```

---

## 9. Build, Flash and Run Steps
1. Add [`src/ring_buffer.c`](src/ring_buffer.c) and [`src/main.c`](src/main.c) to the Keil µVision workspace.
2. Compile and link:
   ```text
   compiling ring_buffer.c...
   compiling main.c...
   linking...
   Program Size: data=43.0 xdata=0 code=284
   creating hex file from "CAT6_TaskB_UART_Echo"...
   "CAT6_TaskB_UART_Echo" - 0 Error(s), 0 Warning(s).
   ```
3. Flash the target:
   ```bash
   avrdude -c usbasp -p 89s52 -U flash:w:CAT6_TaskB_UART_Echo.hex:i
   ```
4. Open PuTTY: **Serial -> COMx -> 9600-8-N-1**. Set "Local echo: Force off".

---

## 10. Expected Output
1. Upon connection/reset, PuTTY displays the startup banner:
   ```text
   =========================================
    8051 UART ECHO CONSOLE READY (9600 8-N-1)
   =========================================
   ```
2. The user types `ABC123`.
3. The PuTTY terminal immediately displays:
   ```text
   ABC123
   ```
4. Typing Enter (`\r`) issues a newline (`\r\n`), advancing the cursor cleanly to the next line.

---

## 11. Viva Q&A (Technical Assessment)
1. **Q: Why must `RI` and `TI` flags be cleared in software inside the 8051 UART ISR?**  
   *A:* The 8051 hardware vectors to address `0x0023` when either `RI` or `TI` is asserted, but does not reset the flag. If software does not explicitly execute `RI = 0` or `TI = 0`, the interrupt condition remains active on exit, immediately re-triggering the ISR and causing an infinite interrupt loop that hangs the system.
2. **Q: How does a ring buffer prevent data loss when receiving burst serial transmissions?**  
   *A:* A ring buffer acts as an asynchronous elastic FIFO. Even if the main CPU loop is temporarily busy processing other tasks when characters arrive over UART, the low-latency ISR immediately pushes bytes into RAM storage. The main loop then dequeues and processes the buffered stream when idle.
3. **Q: What constitutes a "frame error" in UART communication?**  
   *A:* A framing error occurs when the receiver detects an invalid stop bit (e.g. logic `0` instead of logic `1` at the expected stop bit position). This typically indicates a mismatch in baud rate, clock oscillator drift, incorrect parity settings, or signal noise.
4. **Q: What is the maximum throughput possible at 9600 baud with 8-N-1 framing?**  
   *A:* Each character frame consists of 10 bits (1 start, 8 data, 1 stop). At 9600 baud:
   $$\text{Max Throughput} = \frac{9600\text{ bits/sec}}{10\text{ bits/byte}} = 960\text{ bytes/second}$$

---

## 12. Troubleshooting
- **Characters echo twice (e.g. typing `A` displays `AA`):**
  - Disable "Local echo" in PuTTY settings (**Terminal -> Line discipline options -> Local echo -> Force off**).
- **Terminal hangs or stops echoing after typing the first character:**
  - Verify that `tx_busy` is cleared inside the `TI` branch of `uart_isr()`.
  - Verify that `TI = 0;` is executed inside the ISR.

---

## 13. Result and Learning Outcomes
- Successfully implemented an interrupt-driven UART driver on the AT89S52.
- Designed a lock-free 32-byte circular FIFO ring buffer using power-of-2 bitwise masking.
- Demonstrated full-duplex interactive character echo verified via PuTTY serial terminal.
