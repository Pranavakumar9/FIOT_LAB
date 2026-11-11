/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : Timer-Based LED Pattern Generator & Button Controller
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT3 (Week 2 - 27/07/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Pin Mapping:
 *   - P2.0 (Pin 21): LED1 (LSB) -> Active-LOW sink
 *   - P2.1 (Pin 22): LED2       -> Active-LOW sink
 *   - P2.2 (Pin 23): LED3       -> Active-LOW sink
 *   - P2.3 (Pin 24): LED4 (MSB) -> Active-LOW sink
 *   - P3.2 (Pin 12): Momentary Push-Button (/INT0) with external 10k pull-up
 *
 * Timer 0 Configuration:
 *   - Mode 1 (16-bit Timer), F_osc = 11.0592 MHz
 *   - Machine cycle = 1.085069 us
 *   - 1.0 ms overflow requires 922 counts -> Preload = 65536 - 922 = 64614 (0xFC66)
 * ==============================================================================
 */

#include "pattern_fsm.h"

#if defined(__GNUC__)
  /* GCC syntax verification harness */
  static volatile unsigned char P3;
  static volatile unsigned char TMOD;
  static volatile unsigned char TH0;
  static volatile unsigned char TL0;
  static volatile unsigned char IE;
  static volatile unsigned char TCON;
  #define P3_2 1
  #define ET0 1
  #define EX0 1
  #define EA 1
  #define TR0 1
  #define IT0 1
  #define IE0 0
#elif defined(__SDCC)
  #include <8052.h>
  #define PIN_BUTTON P3_2
#else
  #include <reg51.h>
  sbit PIN_BUTTON = P3^2;
#endif

/* Global millisecond timebase counter */
static volatile unsigned int system_time_ms = 0;
static volatile unsigned char hw_button_flag = 0;

/**
 * @brief  Atomic read of the 16-bit millisecond tick counter on 8-bit core.
 */
static unsigned int get_system_ticks(void)
{
    unsigned int ticks;
    #if !defined(__GNUC__)
    EA = 0; /* Temporarily disable interrupts for atomic 16-bit read */
    #endif
    ticks = system_time_ms;
    #if !defined(__GNUC__)
    EA = 1;
    #endif
    return ticks;
}

#if !defined(__GNUC__)
/**
 * @brief  Timer 0 Interrupt Service Routine (Vector 1, address 0x000B).
 *         Fires every 1.0 ms to drive system timebase.
 */
void timer0_isr(void) interrupt 1
{
    /* Reload 16-bit counter for 1.0 ms overflow */
    TH0 = 0xFC;
    TL0 = 0x66;
    system_time_ms++;
}

/**
 * @brief  External Interrupt 0 ISR (Vector 0, address 0x0003).
 *         Fires on falling edge of P3.2 when button is pressed.
 */
void ext0_isr(void) interrupt 0
{
    EX0 = 0;             /* Mask INT0 to mitigate mechanical contact chatter */
    hw_button_flag = 1;  /* Notify debouncing state machine */
}
#endif

/**
 * @brief  Non-blocking mechanical switch debouncing state machine.
 */
static void service_switch_events(void)
{
    typedef enum
    {
        DEBOUNCE_IDLE = 0,
        DEBOUNCE_PRESS_SETTLE,
        DEBOUNCE_WAIT_RELEASE,
        DEBOUNCE_RELEASE_SETTLE
    } debounce_state_t;

    static debounce_state_t db_state = DEBOUNCE_IDLE;
    static unsigned int mark_time = 0;
    unsigned int now = get_system_ticks();

    switch (db_state)
    {
        case DEBOUNCE_IDLE:
            if (hw_button_flag)
            {
                hw_button_flag = 0;
                mark_time = now;
                db_state = DEBOUNCE_PRESS_SETTLE;
            }
            break;

        case DEBOUNCE_PRESS_SETTLE:
            /* Confirm contact closure after 25 ms debounce dwell */
            if ((unsigned int)(now - mark_time) >= 25)
            {
                #if !defined(__GNUC__)
                if (PIN_BUTTON == 0) /* Valid physical press confirmed */
                {
                    fsm_advance_mode();
                    db_state = DEBOUNCE_WAIT_RELEASE;
                }
                else
                {
                    /* False glitch or bounce artifact */
                    IE0 = 0;
                    EX0 = 1;
                    db_state = DEBOUNCE_IDLE;
                }
                #else
                db_state = DEBOUNCE_IDLE;
                #endif
            }
            break;

        case DEBOUNCE_WAIT_RELEASE:
            #if !defined(__GNUC__)
            if (PIN_BUTTON == 1) /* Switch contacts opened */
            {
                mark_time = now;
                db_state = DEBOUNCE_RELEASE_SETTLE;
            }
            #else
            db_state = DEBOUNCE_IDLE;
            #endif
            break;

        case DEBOUNCE_RELEASE_SETTLE:
            /* Confirm stable release after 25 ms */
            if ((unsigned int)(now - mark_time) >= 25)
            {
                #if !defined(__GNUC__)
                if (PIN_BUTTON == 1)
                {
                    IE0 = 0; /* Clear any residual pending flag in TCON */
                    EX0 = 1; /* Re-enable INT0 interrupt */
                    db_state = DEBOUNCE_IDLE;
                }
                else
                {
                    db_state = DEBOUNCE_WAIT_RELEASE;
                }
                #else
                db_state = DEBOUNCE_IDLE;
                #endif
            }
            break;
    }
}

/**
 * @brief  System configuration and peripheral initialization.
 */
static void hardware_setup(void)
{
    #if !defined(__GNUC__)
    /* Configure Timer 0 in Mode 1 (16-bit timer) */
    TMOD = (TMOD & 0xF0) | 0x01;
    TH0 = 0xFC;
    TL0 = 0x66;

    /* Configure INT0 for falling-edge trigger */
    IT0 = 1;
    IE0 = 0;

    /* Configure P3.2 as input by writing logic 1 */
    PIN_BUTTON = 1;

    /* Enable Timer 0 and External 0 interrupts */
    ET0 = 1;
    EX0 = 1;
    EA  = 1;  /* Global interrupt enable */

    /* Start Timer 0 */
    TR0 = 1;
    #endif

    fsm_init();
}

/**
 * @brief  Application entry point.
 */
void main(void)
{
    hardware_setup();

    while (1)
    {
        service_switch_events();
        fsm_update_cadence(get_system_ticks());
    }
}
