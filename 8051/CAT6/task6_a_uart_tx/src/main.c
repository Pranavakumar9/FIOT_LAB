/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : UART Initialization & Periodic String Transmission
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT6 (Week 5 - 17/08/2026) - Task A
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Baud Rate Derivation:
 *   Crystal Frequency = 11.0592 MHz
 *   Timer 1 Clock = 11.0592 MHz / 12 = 921,600 Hz
 *   UART Baud Clock = 921,600 Hz / 32 = 28,800 Hz (with SMOD = 0)
 *   Desired Baud Rate = 9600 Baud
 *   Timer 1 Division Factor = 28,800 / 9600 = 3 counts
 *   Timer 1 Auto-Reload (Mode 2):
 *     TH1 = 256 - 3 = 253 = 0xFD
 *   Exact Baud Error = 0.00% (Zero communication frame errors)
 * ==============================================================================
 */

#if defined(__GNUC__)
  /* GCC syntax verification harness */
  static volatile unsigned char TMOD;
  static volatile unsigned char SCON;
  static volatile unsigned char PCON;
  static volatile unsigned char TH1;
  static volatile unsigned char TL1;
  static volatile unsigned char TH0;
  static volatile unsigned char TL0;
  static volatile unsigned char SBUF;
  static volatile unsigned char TI;
  #define TR1 1
  #define TR0 1
  #define ET0 1
  #define EA 1
#elif defined(__SDCC)
  #include <8052.h>
#else
  #include <reg51.h>
#endif

static volatile unsigned int g_tick_counter = 0;

#if !defined(__GNUC__)
/**
 * @brief  Timer 0 1.0 ms Interrupt Service Routine (for non-blocking pacing)
 */
void timer0_isr(void) interrupt 1
{
    TH0 = 0xFC;
    TL0 = 0x66;
    g_tick_counter++;
}
#endif

/**
 * @brief  Returns safe atomic reading of tick counter.
 */
static unsigned int get_ticks(void)
{
    unsigned int t;
    #if !defined(__GNUC__)
    EA = 0;
    #endif
    t = g_tick_counter;
    #if !defined(__GNUC__)
    EA = 1;
    #endif
    return t;
}

/**
 * @brief  Configures 8051 UART for 9600-8-N-1 using Timer 1 in Mode 2.
 */
static void bsp_uart_init(void)
{
    #if !defined(__GNUC__)
    /*
     * Configure Timers:
     *   Timer 1: Mode 2 (8-bit auto-reload baud generator) -> TMOD high nibble = 0x2
     *   Timer 0: Mode 1 (16-bit periodic timebase)          -> TMOD low nibble  = 0x1
     */
    TMOD = 0x21;

    /* Baud Rate Setup: 9600 Baud @ 11.0592 MHz */
    PCON &= 0x7F; /* SMOD = 0 (Baud rate normal, no doubling) */
    TH1   = 0xFD; /* Reload value for 9600 baud */
    TL1   = 0xFD;

    /*
     * SCON Configuration:
     *   SM0 = 0, SM1 = 1 -> Mode 1 (8-bit UART, variable baud)
     *   SM2 = 0          -> Multiprocessor comms disabled
     *   REN = 1          -> Receiver enabled
     *   TB8 = 0, RB8 = 0
     *   TI = 0, RI = 0
     */
    SCON  = 0x50;

    /* Timer 0 initialization (1.0 ms tick) */
    TH0   = 0xFC;
    TL0   = 0x66;
    ET0   = 1;
    EA    = 1;

    /* Start both timers */
    TR1   = 1;
    TR0   = 1;

    /* Pre-set TI to 1 so first transmission does not block indefinitely */
    TI    = 1;
    #endif
}

/**
 * @brief  Transmits a single byte over UART.
 * @param  ch: ASCII byte to send.
 */
static void bsp_uart_putc(char ch)
{
    #if !defined(__GNUC__)
    /* Wait until transmitter is idle */
    while (!TI);
    TI = 0;
    SBUF = ch;
    #else
    (void)ch;
    #endif
}

/**
 * @brief  Transmits a null-terminated string over UART.
 * @param  str: Pointer to character array in program memory or RAM.
 */
static void bsp_uart_puts(const char *str)
{
    while (*str)
    {
        bsp_uart_putc(*str);
        str++;
    }
}

/**
 * @brief  Main execution entry point.
 */
void main(void)
{
    unsigned int last_tx_time = 0;
    unsigned int now;

    bsp_uart_init();

    while (1)
    {
        now = get_ticks();

        /* Transmit "Hello from 8051" with a clean 1000 ms cadence */
        if ((unsigned int)(now - last_tx_time) >= 1000)
        {
            last_tx_time = now;
            bsp_uart_puts("Hello from 8051\r\n");
        }
    }
}
