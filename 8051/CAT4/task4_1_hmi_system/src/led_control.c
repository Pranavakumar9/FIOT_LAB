/**
 * ==============================================================================
 * @file           : led_control.c
 * @brief          : LED Subsystem Control Routines Implementation
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT4 (Week 3 - 03/08/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#include "led_control.h"

#if defined(__GNUC__)
  static volatile unsigned char P2;
#elif defined(__SDCC)
  #include <8052.h>
#else
  #include <reg51.h>
#endif

/* Dynamic animated mode flags */
typedef enum
{
    MODE_STATIC_HOLD   = 0,
    MODE_RUNNING_CHASE = 5,
    MODE_ALTERNATING   = 6,
    MODE_BINARY_COUNT  = 7,
    MODE_ALL_BLINK     = 8
} led_mode_t;

static led_mode_t active_mode = MODE_STATIC_HOLD;
static unsigned char anim_step = 0;
static unsigned int last_tick_ms = 0;

/**
 * @brief  Writes 4-bit pattern to Port 2 lower nibble (P2.0 - P2.3).
 * @param  pattern: Bitmask where bit=1 illuminates LED (Active-LOW sink).
 */
static void write_bus(unsigned char pattern)
{
    unsigned char sink_val = (~pattern) & 0x0F;
    P2 = (P2 & 0xF0) | sink_val;
}

void led_subsystem_init(void)
{
    active_mode = MODE_STATIC_HOLD;
    anim_step = 0;
    last_tick_ms = 0;
    write_bus(0x00); /* Extinguish all LEDs */
}

void running_led(void)
{
    /* Circular chase: 0001 -> 0010 -> 0100 -> 1000 */
    write_bus(1 << (anim_step & 0x03));
    anim_step = (anim_step + 1) & 0x03;
}

void binary_counter(void)
{
    /* 4-bit binary up-count: 0000 to 1111 */
    write_bus(anim_step & 0x0F);
    anim_step = (anim_step + 1) & 0x0F;
}

void alternate_led(void)
{
    /* Alternating 1010 <-> 0101 */
    if (anim_step == 0)
    {
        write_bus(0x0A);
        anim_step = 1;
    }
    else
    {
        write_bus(0x05);
        anim_step = 0;
    }
}

void blink_all(void)
{
    /* All LEDs flash: 1111 <-> 0000 */
    if (anim_step == 0)
    {
        write_bus(0x0F);
        anim_step = 1;
    }
    else
    {
        write_bus(0x00);
        anim_step = 0;
    }
}

void led_control(unsigned char key)
{
    switch (key)
    {
        case '1':
            active_mode = MODE_STATIC_HOLD;
            write_bus(0x01); /* LED1 ON */
            break;

        case '2':
            active_mode = MODE_STATIC_HOLD;
            write_bus(0x02); /* LED2 ON */
            break;

        case '3':
            active_mode = MODE_STATIC_HOLD;
            write_bus(0x04); /* LED3 ON */
            break;

        case '4':
            active_mode = MODE_STATIC_HOLD;
            write_bus(0x08); /* LED4 ON */
            break;

        case '5':
            active_mode = MODE_RUNNING_CHASE;
            anim_step = 0;
            running_led();
            break;

        case '6':
            active_mode = MODE_ALTERNATING;
            anim_step = 0;
            alternate_led();
            break;

        case '7':
            active_mode = MODE_BINARY_COUNT;
            anim_step = 0;
            binary_counter();
            break;

        case '8':
            active_mode = MODE_ALL_BLINK;
            anim_step = 0;
            blink_all();
            break;

        case '9':
            active_mode = MODE_STATIC_HOLD;
            write_bus(0x00); /* All LEDs OFF */
            break;

        case '0':
            /* Reset system: clear all LEDs */
            active_mode = MODE_STATIC_HOLD;
            anim_step = 0;
            write_bus(0x00);
            break;

        default:
            break;
    }
}

void led_service_animation(unsigned int current_ms)
{
    unsigned int interval_ms;

    if (active_mode == MODE_STATIC_HOLD)
    {
        return;
    }

    switch (active_mode)
    {
        case MODE_RUNNING_CHASE:
            interval_ms = 250;
            break;
        case MODE_BINARY_COUNT:
            interval_ms = 400;
            break;
        case MODE_ALTERNATING:
            interval_ms = 350;
            break;
        case MODE_ALL_BLINK:
            interval_ms = 200;
            break;
        default:
            interval_ms = 300;
            break;
    }

    if ((unsigned int)(current_ms - last_tick_ms) >= interval_ms)
    {
        last_tick_ms = current_ms;

        switch (active_mode)
        {
            case MODE_RUNNING_CHASE:
                running_led();
                break;
            case MODE_BINARY_COUNT:
                binary_counter();
                break;
            case MODE_ALTERNATING:
                alternate_led();
                break;
            case MODE_ALL_BLINK:
                blink_all();
                break;
            default:
                break;
        }
    }
}
