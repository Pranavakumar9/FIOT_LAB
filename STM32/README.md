# STM32 32-Bit ARM Embedded Systems Laboratory Sequence (CAT 8 – CAT 13)

**Course:** 25CS2741 – Fundamentals of Internet of Things + Lab  
**Institution:** Shiv Nadar University – Chennai  
**Department:** Computer Science & Engineering (IoT Specialization)  
**Target Architectures:**
- STMicroelectronics STM32F103C8T6 "Blue Pill" (ARM Cortex-M3 @ 72 MHz)
- STMicroelectronics NUCLEO-F401RE (ARM Cortex-M4 @ 84 MHz with FPU)  
**Author:** Pranavakumar  

---

## 1. Overview and Architecture Hierarchy

The STM32 curriculum transitions students from legacy 8-bit microcontrollers to 32-bit ARM Cortex-M microcontrollers and connected IoT edge systems. The laboratory progression spans bare-metal GPIO manipulation, software finite state machines for switch debouncing, line-buffered interrupt serial parsers, coprocessor Wi-Fi integration via Hayes AT commands, I2C sensor hardware abstraction layers (Bosch Sensortec BME280), embedded REST Web Servers, and lightweight MQTT 3.1.1 telemetry publication feeding cloud analytics dashboards in Node-RED.

```
STM32/
├── CAT8/     # STM32F103 GPIO Bring-Up, FSM Debouncing & Press Duration Detection
│   ├── task8_a/   # Cortex-M3 GPIO Bring-Up (Active-LOW PC13 LED & SysTick Delays)
│   ├── task8_b/   # Software Debouncing Finite State Machine (PA0 Pushbutton)
│   └── task8_c/   # Press Duration Classifier (Short < 1s vs Long >= 1s)
├── CAT9/     # STM32F103 USART1 Full-Duplex Serial Communication
│   ├── task9_a/   # Polled vs Interrupt Echo Server (USART1 @ 115200 Baud)
│   └── task9_b/   # Line-Buffered Command Parser (LED_ON, LED_OFF, STATUS)
├── CAT10/    # STM32F103 + ESP8266 AT Modem Bridge & Multi-Connection HTTP Server
│   ├── task10_a/  # Serial Bridge (USART1 ST-LINK <-> USART2 ESP8266 AT Passthrough)
│   ├── task10_b/  # Wi-Fi Station Provisioning & DHCP Query (AT+CWMODE, AT+CWJAP)
│   ├── task10_c/  # Embedded Multi-Connection TCP Web Server (Port 80)
│   ├── task10_d/  # Two-Step AT+CIPSEND HTTP Payload Dispatcher
│   └── task10_e/  # Node-RED HTTP Client Automation Flow
├── CAT11/    # NUCLEO-F401RE Cortex-M4 GPIO Bring-Up & Duration Detection
│   ├── task11_1/  # Cortex-M4 GPIO Bring-Up (Active-HIGH PA5 LD2 & SysTick)
│   ├── task11_2/  # Switch State Remembrance with Software Debouncing (PC13 B1)
│   └── task11_3/  # Duration-Based Lighting Sequencer (Short < 1s vs Long >= 1s)
├── CAT12/    # NUCLEO-F401RE + BME280 I2C Bosch Sensor API & HTTP JSON Server
│   ├── task12_a_bme280_i2c/       # Bosch Sensortec Sensor API Driver over I2C1 (PB6/PB7)
│   ├── task12_b_http_json/        # Embedded HTTP JSON Server with Dynamic Content-Length
│   └── task12_c_nodered_analytics/# Node-RED HTTP Poller Computing Running Min/Max/Avg
└── CAT13/    # NUCLEO-F401RE + BME280 I2C + Custom ESPLib MQTT Client & Node-RED
    ├── task13_a_mqtt_client/      # Custom ESPLib MQTT 3.1.1 Client Driver Bring-Up
    ├── task13_b_telemetry_pub/    # BME280 I2C Sensor to MQTT Telemetry Publisher (1 Hz)
    └── task13_c_nodered_dash/     # Node-RED MQTT Cloud Dashboard & Telemetry Visualizer
```

---

## 2. Laboratory Task Index

| CAT | Directory | Platform | Key Hardware / Protocols | Primary Focus |
|---|---|---|---|---|
| **CAT 8** | [`CAT8/`](./CAT8/) | STM32F103 | PC13 LED, PA0 Button | Software debouncing FSM, press duration timing via `HAL_GetTick()`. |
| **CAT 9** | [`CAT9/`](./CAT9/) | STM32F103 | USART1 (115200 baud) | Line-buffered serial parser, CRLF frame detection, command dispatcher. |
| **CAT 10** | [`CAT10/`](./CAT10/) | STM32F103 | ESP8266, TCP Port 80 | Full Hayes AT state machine, multi-client web server, Node-RED polling. |
| **CAT 11** | [`CAT11/`](./CAT11/) | NUCLEO-F401RE | PA5 LD2, PC13 B1 | Cortex-M4 84 MHz migration, Active-HIGH LED logic, dual-mode timing. |
| **CAT 12** | [`CAT12/`](./CAT12/) | NUCLEO-F401RE | BME280 (I2C), ESP8266 | Bosch Sensor API, dynamic `Content-Length`, HTTP REST JSON server. |
| **CAT 13** | [`CAT13/`](./CAT13/) | NUCLEO-F401RE | BME280, ESP8266, MQTT | Binary MQTT 3.1.1 frames via `ESPLib`, cloud pub/sub, Node-RED UI. |

---

## 3. Platform Architecture and Hardware Differences

| Parameter | STM32F103C8T6 "Blue Pill" (CAT 8–10) | STM32 NUCLEO-F401RE (CAT 11–13) |
|---|---|---|
| **Core Architecture** | ARM Cortex-M3 (32-bit RISC) | ARM Cortex-M4 with single-precision FPU |
| **Maximum Clock Frequency** | 72 MHz (8 MHz HSE with PLL) | 84 MHz (16 MHz HSI with PLL) |
| **Flash / SRAM** | 64 KB Flash / 20 KB SRAM | 512 KB Flash / 96 KB SRAM |
| **On-Board User LED** | Pin **PC13** – **Active-LOW** (0 = ON) | Pin **PA5** (LD2) – **Active-HIGH** (1 = ON) |
| **On-Board Pushbutton** | External breadboard button (PA0) | Pin **PC13** (B1) – **Active-LOW** (0 = Pressed) |
| **I2C Peripheral** | I2C1 (PB6 SCL, PB7 SDA) | I2C1 (PB6 SCL, PB7 SDA) |
| **Primary Debug Interface** | USART1 (PA9 TX, PA10 RX) | USART2 (PA2 TX, PA3 RX via ST-LINK VCP) |
| **Target Application** | Foundation embedded control | High-speed IoT gateway & sensor analytics |

---

## 4. Key Engineering Standards and Software Modules

### 4.1 Bosch Sensortec BME280 Sensor API
Rather than hardcoding fragile register manipulation and floating-point approximations, CAT 12 and CAT 13 use the official Bosch Sensortec Sensor API driver (`bme280.c`, `bme280.h`). The driver uses clean C function pointers:
```c
struct bme280_dev dev;
dev.intf = BME280_I2C_INTF;
dev.read = user_i2c_read;
dev.write = user_i2c_write;
dev.delay_us = user_delay_us;
bme280_init(&dev);
```
All factory trim calibration values are loaded automatically, computing bit-exact compensated temperature, pressure, and humidity.

### 4.2 Custom Embedded MQTT 3.1.1 Driver (`ESPLib`)
In CAT 13, the custom-built `ESPLib` library constructs binary MQTT frames directly in microcontroller SRAM:
- Connect Packet (`0x10`) with CleanSession flag and KeepAlive timer.
- Publish Packet (`0x30`) with QoS 0, UTF-8 topic encoding, and JSON payload.
- SysTick integration calling `MQTTTimer()` every 1 ms to service background keep-alive pings.

### 4.3 Node-RED Low-Code IoT Orchestration
The laboratory series integrates Node-RED for cloud and edge data analytics:
- **HTTP REST Architecture (CAT 10, CAT 12):** Polling embedded web servers at regular intervals, computing statistical running Min, Max, and Average in function nodes.
- **MQTT Event Architecture (CAT 13):** Asynchronous event-driven push model ingesting telemetry from `broker.emqx.io:1883`, displaying instantaneous radial gauges, and plotting rolling historical trends.

---

## 5. Build, Flash, and Verification Instructions

All C code in the STM32 sequence is fully compliant with ARM GCC, Keil MDK-ARM, and STM32CubeIDE.

### Headless Verification with GCC
Every source file in this repository is tested against GCC 13.2.0 (UCRT64) with `-Wall -Wextra -fsyntax-only` to guarantee 0 compiler errors and 0 warnings:
```bash
gcc -fsyntax-only -Wall -Wextra -I/path/to/test_hal_f4 -I./src src/*.c
```

### Flashing via ST-LINK
Connect the STM32 board via USB. Using STM32CubeProgrammer or dragging and dropping the compiled `.bin` file into the mounted mass storage volume flashes the firmware immediately without requiring external bootloaders.
