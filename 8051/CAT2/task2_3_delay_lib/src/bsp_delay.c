/**
 * ==============================================================================
 * @file           : bsp_delay.c
 * @brief          : Board Support Package Software Delay Implementation
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT2 (Week 2 - 20/07/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Machine Cycle Analysis:
 *   Crystal Oscillator = 11.0592 MHz
 *   1 Machine Cycle (T_mc) = 12 / 11,059,200 Hz = 1.085069 microseconds
 *   Number of machine cycles in 1 ms = 1000 us / 1.085069 us = 921.6 cycles
 *   The inner DJNZ/loop sequence takes 8 cycles per iteration,
 *   yielding 921.6 / 8 = ~115 loop counts per millisecond.
 * ==============================================================================
 */

#include "bsp_delay.h"

#if defined(__GNUC__)
  #define _nop_() __asm__ volatile ("nop")
#elif defined(__SDCC)
  #define _nop_() __asm nop __endasm
#else
  #include <intrins.h>
#endif

void bsp_delay_ms(unsigned int ms)
{
    unsigned int outer_idx;
    unsigned char inner_idx;

    for (outer_idx = 0; outer_idx < ms; outer_idx++)
    {
        for (inner_idx = 0; inner_idx < 115; inner_idx++)
        {
            _nop_();
            _nop_();
        }
    }
}

void bsp_delay_us(unsigned int us)
{
    /* At 1.085 us per cycle, each microsecond corresponds to ~1 machine cycle */
    while (us--)
    {
        _nop_();
    }
}
