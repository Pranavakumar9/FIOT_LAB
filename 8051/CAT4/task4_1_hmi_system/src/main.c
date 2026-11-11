/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : HMI System integrating Keypad, 7-Segment, and LED Bus
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT4 (Week 3 - 03/08/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Port Assignments:
 *   - Port 1 (P1.0 - P1.7): 4x4 Matrix Keypad (P1.0-P1.3 Rows, P1.4-P1.7 Cols)
 *   - Port 0 (P0.0 - P0.7): Common-Anode 7-Segment (with 10k pull-up resistor pack)
 *   - Port 2 (P2.0 - P2.3): 4x Mode Indicator LEDs (Active-LOW sink via 330R)
 * ==============================================================================
 */

#include "keypad.h"
#include "seven_seg.h"
#include "led_control.h"

#if defined(__GNUC__)
  /* GCC syntax verification harness */
  static volatile unsigned char TMOD;
  static volatile unsigned char TH0;
  static volatile unsigned char TL0;
  #define ET0 1
  #define EA 1
  #define TR0 1
#elif defined(__SDCC)
  #include <8052.h>
#else
  #include <reg51.h>
#endif

/* Global millisecond timebase */
static volatile unsigned int sys_ms_ticks = 0;

/**
 * @brief  Returns safe atomic reading of 16-bit millisecond tick count.
 */
static unsigned int get_system_ms(void)
{
    unsigned int ms;
    #if !defined(__GNUC__)
    EA = 0;
    #endif
    ms = sys_ms_ticks;
    #if !defined(__GNUC__)
    EA = 1;
    #endif
    return ms;
}

#if !defined(__GNUC__)
/**
 * @brief  Timer 0 1.0 ms Interrupt Service Routine
 *         11.0592 MHz crystal -> Preload = 65536 - 922 = 0xFC66
 */
void timer0_isr(void) interrupt 1
{
    TH0 = 0xFC;
    TL0 = 0x66;
    sys_ms_ticks++;
}
#endif

/**
 * @brief  Scans matrix keypad, updates 7-segment display, and executes LED command.
 * @note   Implements debounced stateful edge detection.
 */
void keypad_scan(void)
{
    static unsigned char prev_key = NO_KEY_PRESSED;
    static unsigned int debounce_timestamp = 0;
    unsigned char current_key = keypad_get_key();
    unsigned int now = get_system_ms();

    if (current_key != prev_key)
    {
        /* Check if debounce window has elapsed */
        if ((unsigned int)(now - debounce_timestamp) >= 30)
        {
            debounce_timestamp = now;
            prev_key = current_key;

            if (current_key != NO_KEY_PRESSED)
            {
                if (current_key >= '0' && current_key <= '9')
                {
                    /* Visual feedback: render digit on 7-segment */
                    display_digit(current_key - '0');

                    /* Functional execution: update LED operating mode */
                    led_control(current_key);
                }
            }
        }
    }
}

/**
 * @brief  Hardware peripheral bring-up and timer setup.
 */
static void system_hardware_init(void)
{
    #if !defined(__GNUC__)
    /* Timer 0 Mode 1 (16-bit) */
    TMOD = (TMOD & 0xF0) | 0x01;
    TH0 = 0xFC;
    TL0 = 0x66;

    ET0 = 1; /* Enable Timer 0 Interrupt */
    EA  = 1; /* Enable Global Interrupts */
    TR0 = 1; /* Start Timer 0 */
    #endif

    keypad_init();
    seven_seg_init();
    led_subsystem_init();

    /* Initial state: display digit 0 and all LEDs OFF */
    display_digit(0);
    led_control('0');
}

/**
 * @brief  Main execution entry point.
 */
void main(void)
{
    system_hardware_init();

    while (1)
    {
        keypad_scan();
        led_service_animation(get_system_ms());
    }
}
