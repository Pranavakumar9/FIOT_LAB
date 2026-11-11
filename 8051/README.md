# 8051 Embedded Systems Laboratory Sequence (CAT 2 – CAT 7)

**Course:** 25CS2741 – Fundamentals of Internet of Things + Lab  
**Institution:** Shiv Nadar University – Chennai  
**Department:** Computer Science & Engineering (IoT Specialization)  
**Target Architecture:** Intel MCS-51 / Microchip AT89S52 (40-Pin DIP, 8-bit CISC)  
**Author:** Pranavakumar  

---

## 1. Overview and Curriculum Architecture

The 8051 curriculum sequence covers register-level embedded C programming on the classical 8051 microcontroller architecture using the Keil C51 compiler and SDCC. All implementations strictly adhere to bare-metal register operations without third-party frameworks, RTOS, or Arduino abstractions.

```
8051/
├── CAT2/      # LED Bring-Up, Active-LOW Blink & Calibrated Delay Library (11.0592 MHz)
│   ├── task2_1/   # Basic Register-Level LED Blink (P1.0)
│   ├── task2_2/   # Calibrated Millisecond Delay Generator
│   └── task2_3/   # Frequency-Swept Multi-LED Alternating Pattern
├── CAT3/      # Hardware Timer Modes & External Hardware Interrupts (INT0)
│   └── task3_1/   # 4-Mode Timer State Machine & Debounced INT0 Pushbutton
├── CAT4/      # Matrix Keypad Interfacing & 7-Segment Multiplexing
│   └── task4_1/   # 4x4 Keypad Matrix Scanner, 7-Segment Decoder & Dynamic Patterns
├── CAT5/      # Precision 1ms Timebase, Software PWM Fading & Actuator Control
│   ├── task5_1/   # 1 kHz Timer Timebase & Variable Duty-Cycle PWM LED Fading
│   ├── task5_2/   # Timer-Driven Square & Triangular Wave Generator
│   └── task5_3/   # ULN2003 Stepper Motor 4-Phase Step Sequencer
├── CAT6/      # Full-Duplex Serial Communication (UART) & Circular Buffers
│   ├── task6_a/   # 9600-8-N-1 Polled UART Transmitter with SBUF/TI Handshake
│   └── task6_b/   # Interrupt-Driven Circular Ring Buffer UART Receiver & Echo
└── CAT7/      # 16x2 Alphanumeric HD44780 LCD & Telemetry Navigation
    ├── task7_a/   # 4-bit Nibble-Mode HD44780 LCD Driver with Custom CGRAM Glyphs
    ├── task7_b/   # Dynamic Telemetry Display with Live Uptime Counter
    └── task7_c/   # Menu Navigation System Combining 4x4 Keypad and 16x2 LCD
```

---

## 2. Laboratory Task Index

| CAT | Directory | Key Hardware | Description |
|---|---|---|---|
| **CAT 2** | [`CAT2/`](./CAT2/) | AT89S52, LEDs | Active-LOW GPIO driving on Port 1, cycle-exact delay loops for 11.0592 MHz crystal. |
| **CAT 3** | [`CAT3/`](./CAT3/) | Timer 0/1, INT0 | 4 operational modes toggled via hardware external interrupt `INT0` (P3.2) with RC debouncing. |
| **CAT 4** | [`CAT4/`](./CAT4/) | 4x4 Keypad, 7-Segment | Row-scanning column-reading keypad debouncing, Common-Cathode 7-segment display driver. |
| **CAT 5** | [`CAT5/`](./CAT5/) | ULN2003, 28BYJ-48 Stepper | 1 ms Timer 0 timebase, 500 Hz PWM software fading, and unipolar stepper motor half-stepping. |
| **CAT 6** | [`CAT6/`](./CAT6/) | UART, MAX232 | Mode 1 8-bit UART @ 9600 baud (`TH1 = 0xFD`), circular FIFO buffer with head/tail pointers. |
| **CAT 7** | [`CAT7/`](./CAT7/) | HD44780 LCD, Keypad | 4-bit bus interface (RS, EN, D4–D7), dynamic telemetry display, interactive menu tree. |

---

## 3. Core Technical Concepts and Mathematical Formulations

### 3.1 Machine Cycle and Instruction Timing
On standard 12-clock 8051 cores with crystal frequency $F_{osc} = 11.0592\text{ MHz}$:

$$F_{machine} = \frac{F_{osc}}{12} = \frac{11.0592\text{ MHz}}{12} = 921.6\text{ kHz}$$

$$T_{cycle} = \frac{1}{921.6\text{ kHz}} \approx 1.08507\ \mu\text{s}$$

For a 12.0000 MHz crystal:

$$F_{machine} = \frac{12.0\text{ MHz}}{12} = 1.0\text{ MHz} \implies T_{cycle} = 1.0\ \mu\text{s}$$

### 3.2 Hardware Timer Reload Calculations (Mode 1 - 16-bit Timer)
To generate an interrupt every $\Delta t$ seconds:

$$N_{counts} = \frac{\Delta t}{T_{cycle}}$$

$$\text{Reload Value} = 65536 - N_{counts}$$

$$\text{THx} = \lfloor\text{Reload} / 256\rfloor, \quad \text{TLx} = \text{Reload} \pmod{256}$$

For $\Delta t = 1.0\text{ ms}$ at $11.0592\text{ MHz}$:
- $N_{counts} = \frac{1000\ \mu\text{s}}{1.08507\ \mu\text{s}} = 921.6 \approx 922$ counts.
- $\text{Reload} = 65536 - 922 = 64614 = \text{0xFC66}$.
- $\text{TH0} = \text{0xFC}$, $\text{TL0} = \text{0x66}$.

### 3.3 UART Baud Rate Generation (Timer 1 Mode 2 - 8-bit Auto-Reload)
When `SMOD = 0` (standard baud rate):

$$\text{Baud Rate} = \frac{F_{osc}}{384 \times (256 - \text{TH1})}$$

Solving for $\text{TH1}$ given Baud Rate $= 9600$ and $F_{osc} = 11.0592\text{ MHz}$:

$$256 - \text{TH1} = \frac{11,059,200}{384 \times 9600} = \frac{11,059,200}{3,686,400} = 3 \implies \text{TH1} = 253 = \text{0xFD}$$

The 11.0592 MHz crystal yields a **0.00% baud rate error**, eliminating UART framing errors.

---

## 4. Toolchain Setup and Compilation Guidelines

### Keil µVision C51
1. Create a project targeting `AT89S52`.
2. Ensure `STARTUP.A51` is included in project tree.
3. Select **Options for Target -> Output -> Create HEX File**.
4. Build target (`F7`) and flash using an ISP programmer (e.g., USBASP via AVRDUDE or ProgISP).

### SDCC (Small Device C Compiler)
To compile any sub-task using open-source SDCC:
```bash
sdcc -mmcs51 --model-small main.c
packihx main.ihx > main.hex
```

Flash HEX file via USBASP:
```bash
avrdude -c usbasp -p at89s52 -U flash:w:main.hex:i
```
