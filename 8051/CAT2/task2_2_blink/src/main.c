/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : Bare-Metal LED Blinking Application using Direct SFR Access
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT2 (Week 2 - 20/07/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * @description
 * Implements a bare-metal register-level GPIO driver to toggle a diagnostic LED
 * at a symmetric 1.0 Hz cadence (500 ms ON, 500 ms OFF).
 *
 * Electrical Configuration:
 *   - Pin: Port 2, Bit 0 (P2.0, Pin 21 on DIP-40)
 *   - Topology: Active-LOW (Sink current drive).
 *     * Anode is tied to +5V VCC via a 330 Ohm series resistor.
 *     * Cathode is tied to P2.0.
 *     * 8051 quasi-bidirectional ports sink up to 20 mA (Logic 0),
 *       providing strong drive without needing an external transistor.
 *
 * Timing Derivation (11.0592 MHz Crystal):
 *   - F_osc = 11.0592 MHz
 *   - Machine Cycle T_cy = 12 / F_osc = 1.085069 microseconds
 *   - Inner calibrated software loop provides ~1 ms per iteration block.
 * ==============================================================================
 */

#if defined(__GNUC__)
  /* GCC syntax verification harness */
  static volatile unsigned char P2;
  static volatile unsigned char PIN_LED;
  #define _nop_() __asm__ volatile ("nop")
#elif defined(__SDCC)
  #include <8052.h>
  #define PIN_LED P2_0
  #define _nop_() __asm nop __endasm
#else
  #include <reg51.h>
  #include <intrins.h>
  sbit PIN_LED = P2^0;
#endif

/* Forward declarations */
static void software_delay_approx_ms(unsigned int duration_ms);

/**
 * @brief  Software delay loop calibrated for ~11.0592 MHz crystal.
 * @param  duration_ms: Milliseconds to dwell in CPU busy loop.
 * @note   Inner loop executes approximately 122 iterations of DJNZ/NOP sequences
 *         corresponding to ~922 machine cycles (~1.0 ms).
 */
static void software_delay_approx_ms(unsigned int duration_ms)
{
    unsigned int count_outer;
    unsigned char count_inner;

    for (count_outer = 0; count_outer < duration_ms; count_outer++)
    {
        /* ~1 ms inner loop for 11.0592 MHz MCS-51 core */
        for (count_inner = 0; count_inner < 122; count_inner++)
        {
            _nop_();
        }
    }
}

/**
 * @brief  Application entry point.
 */
void main(void)
{
    /* Step 1: Ensure initial state is OFF (Active-LOW LED: 1 = OFF, 0 = ON) */
    PIN_LED = 1;

    /* Step 2: Superloop blinking at ~1 Hz (500 ms ON, 500 ms OFF) */
    while (1)
    {
        /* Turn LED ON by pulling P2.0 LOW (sinking current) */
        PIN_LED = 0;
        software_delay_approx_ms(500);

        /* Turn LED OFF by pulling P2.0 HIGH */
        PIN_LED = 1;
        software_delay_approx_ms(500);
    }
}
