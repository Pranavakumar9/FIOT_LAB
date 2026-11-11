/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : Demonstration of Modular Delay Library with Heartbeat Cadence
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT2 (Week 2 - 20/07/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#include "bsp_delay.h"

#if defined(__GNUC__)
  /* GCC syntax verification harness */
  static volatile unsigned char PIN_STATUS_LED;
#elif defined(__SDCC)
  #include <8052.h>
  #define PIN_STATUS_LED P2_0
#else
  #include <reg51.h>
  sbit PIN_STATUS_LED = P2^0;
#endif

/**
 * @brief  Main execution loop demonstrating delay library accuracy via
 *         a dual-pulse heartbeat optical telemetry pattern:
 *         Pulse 1 (150 ms) -> Gap (150 ms) -> Pulse 2 (150 ms) -> Rest (800 ms)
 */
void main(void)
{
    /* Initialize Active-LOW LED in OFF state */
    PIN_STATUS_LED = 1;
    bsp_delay_ms(100);

    while (1)
    {
        /* First beat */
        PIN_STATUS_LED = 0;   /* LED ON */
        bsp_delay_ms(150);
        PIN_STATUS_LED = 1;   /* LED OFF */
        bsp_delay_ms(150);

        /* Second beat */
        PIN_STATUS_LED = 0;   /* LED ON */
        bsp_delay_ms(150);
        PIN_STATUS_LED = 1;   /* LED OFF */
        bsp_delay_ms(800);    /* Inter-heartbeat resting interval */
    }
}
