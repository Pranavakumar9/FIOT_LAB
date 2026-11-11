/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : Timer 0 Non-Blocking 1.0 ms Time Base and 1-Second LED Toggle
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT5 (Week 4 - 10/08/2026) - Task 1
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Mathematical Derivation:
 *   Crystal Frequency F_osc = 11.0592 MHz
 *   Machine Cycle Period T_mc = 12 / 11.0592 MHz = 1.085069 microseconds
 *   Target Interrupt Interval = 1.000 ms = 1000 microseconds
 *   Required Count N = 1000 us / 1.085069 us = 921.6 ~ 922 counts
 *   16-Bit Preload = 65536 - 922 = 64614 = 0xFC66
 *   TH0 Reload = 0xFC, TL0 Reload = 0x66
 * ==============================================================================
 */

#if defined(__GNUC__)
  /* GCC syntax verification harness */
  static volatile unsigned char TMOD;
  static volatile unsigned char TH0;
  static volatile unsigned char TL0;
  static volatile unsigned char PIN_LED;
  #define ET0 1
  #define EA 1
  #define TR0 1
#elif defined(__SDCC)
  #include <8052.h>
  #define PIN_LED P1_0
#else
  #include <reg51.h>
  sbit PIN_LED = P1^0;
#endif

/* Global millisecond counter updated by hardware Timer 0 ISR */
static volatile unsigned int g_system_tick_ms = 0;

/**
 * @brief  Returns safe atomic snapshot of 16-bit tick counter.
 */
static unsigned int get_tick_ms(void)
{
    unsigned int tick_snapshot;
    #if !defined(__GNUC__)
    EA = 0; /* Mask interrupts for atomic 16-bit read on 8-bit core */
    #endif
    tick_snapshot = g_system_tick_ms;
    #if !defined(__GNUC__)
    EA = 1; /* Restore interrupts */
    #endif
    return tick_snapshot;
}

#if !defined(__GNUC__)
/**
 * @brief  Timer 0 Interrupt Service Routine (Vector 1, 0x000B).
 *         Fires precisely every 1.00 ms.
 */
void timer0_isr(void) interrupt 1
{
    /* Reload high and low bytes for 1.0 ms period */
    TH0 = 0xFC;
    TL0 = 0x66;
    g_system_tick_ms++;
}
#endif

/**
 * @brief  Hardware initialization for Timer 0.
 */
static void timer0_configuration(void)
{
    #if !defined(__GNUC__)
    /* Set Timer 0 in 16-bit mode (Mode 1): Gate=0, C/T=0, M1=0, M0=1 */
    TMOD = (TMOD & 0xF0) | 0x01;

    /* Initialize reload values */
    TH0 = 0xFC;
    TL0 = 0x66;

    /* Enable Timer 0 Interrupt (ET0) and Global Interrupts (EA) */
    ET0 = 1;
    EA  = 1;

    /* Run Timer 0 */
    TR0 = 1;
    #endif

    PIN_LED = 1; /* Initial state: LED OFF (Active-LOW sink) */
}

/**
 * @brief  Main non-blocking execution loop.
 */
void main(void)
{
    unsigned int previous_toggle_time = 0;
    unsigned int current_time;

    timer0_configuration();

    while (1)
    {
        current_time = get_tick_ms();

        /* Non-blocking delta time comparison for 1000 ms (1 second) interval */
        if ((unsigned int)(current_time - previous_toggle_time) >= 1000)
        {
            previous_toggle_time = current_time;

            /* Toggle LED output state */
            PIN_LED = !PIN_LED;
        }
    }
}
