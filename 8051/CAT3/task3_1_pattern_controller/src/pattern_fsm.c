/**
 * ==============================================================================
 * @file           : pattern_fsm.c
 * @brief          : Finite State Machine Implementation for LED Patterns
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT3 (Week 2 - 27/07/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#include "pattern_fsm.h"

#if defined(__GNUC__)
  static volatile unsigned char P2;
  #define ROM_CODE
#elif defined(__SDCC)
  #include <8052.h>
  #define ROM_CODE __code
#else
  #include <reg51.h>
  #define ROM_CODE code
#endif

/* Look-up tables stored in program ROM to conserve 8051 on-chip RAM */
static const unsigned char ROM_CODE CHASER_TABLE[4] = {
    0x01,  /* 0001: LED1 ON */
    0x02,  /* 0010: LED2 ON */
    0x04,  /* 0100: LED3 ON */
    0x08   /* 1000: LED4 ON */
};

static const unsigned char ROM_CODE ALTERNATING_TABLE[2] = {
    0x0A,  /* 1010: LED2 & LED4 ON */
    0x05   /* 0101: LED1 & LED3 ON */
};

static const unsigned char ROM_CODE STROBE_TABLE[2] = {
    0x0F,  /* 1111: All LEDs ON */
    0x00   /* 0000: All LEDs OFF */
};

/* Internal state tracking */
static sys_mode_t current_mode = SYS_MODE_OFF;
static unsigned char step_index = 0;
static unsigned int last_update_ms = 0;

/**
 * @brief  Writes 4-bit pattern to Port 2 lower nibble (P2.0 - P2.3).
 * @param  pattern: Bitmask where 1 represents LED ON, 0 represents LED OFF.
 * @note   Active-LOW hardware sinking topology: logic 1 in pattern pulls pin LOW.
 */
static void write_led_bus(unsigned char pattern)
{
    unsigned char inverted_nibble = (~pattern) & 0x0F;
    P2 = (P2 & 0xF0) | inverted_nibble;
}

void fsm_init(void)
{
    current_mode = SYS_MODE_OFF;
    step_index = 0;
    last_update_ms = 0;
    write_led_bus(0x00); /* Extinguish all LEDs */
}

void fsm_advance_mode(void)
{
    step_index = 0;
    last_update_ms = 0;

    switch (current_mode)
    {
        case SYS_MODE_OFF:
            current_mode = SYS_MODE_RUNNING;
            break;
        case SYS_MODE_RUNNING:
            current_mode = SYS_MODE_BINARY;
            break;
        case SYS_MODE_BINARY:
            current_mode = SYS_MODE_ALTERNATE;
            break;
        case SYS_MODE_ALTERNATE:
            current_mode = SYS_MODE_FLASH_ALL;
            break;
        case SYS_MODE_FLASH_ALL:
            current_mode = SYS_MODE_RUNNING; /* Loop back to Mode 1 per spec */
            break;
        default:
            current_mode = SYS_MODE_RUNNING;
            break;
    }
}

void fsm_update_cadence(unsigned int current_ms)
{
    unsigned int dwell_interval_ms;
    unsigned char output_pattern = 0x00;

    if (current_mode == SYS_MODE_OFF)
    {
        write_led_bus(0x00);
        return;
    }

    /* Assign cadence intervals tailored for distinct visual perception */
    switch (current_mode)
    {
        case SYS_MODE_RUNNING:
            dwell_interval_ms = 250; /* 250 ms shift rate */
            break;
        case SYS_MODE_BINARY:
            dwell_interval_ms = 400; /* 400 ms counter increment */
            break;
        case SYS_MODE_ALTERNATE:
            dwell_interval_ms = 350; /* 350 ms alternating flip */
            break;
        case SYS_MODE_FLASH_ALL:
            dwell_interval_ms = 200; /* 200 ms fast strobe */
            break;
        default:
            dwell_interval_ms = 300;
            break;
    }

    /* Check non-blocking elapsed time */
    if ((unsigned int)(current_ms - last_update_ms) >= dwell_interval_ms)
    {
        last_update_ms = current_ms;

        switch (current_mode)
        {
            case SYS_MODE_RUNNING:
                output_pattern = CHASER_TABLE[step_index];
                step_index = (step_index + 1) & 0x03; /* Modulo 4 */
                break;

            case SYS_MODE_BINARY:
                output_pattern = step_index & 0x0F;
                step_index = (step_index + 1) & 0x0F; /* Modulo 16 (0 to 15) */
                break;

            case SYS_MODE_ALTERNATE:
                output_pattern = ALTERNATING_TABLE[step_index];
                step_index = (step_index + 1) & 0x01; /* Modulo 2 */
                break;

            case SYS_MODE_FLASH_ALL:
                output_pattern = STROBE_TABLE[step_index];
                step_index = (step_index + 1) & 0x01; /* Modulo 2 */
                break;

            default:
                output_pattern = 0x00;
                break;
        }

        write_led_bus(output_pattern);
    }
}

sys_mode_t fsm_get_active_mode(void)
{
    return current_mode;
}
