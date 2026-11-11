/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : Interrupt-Driven UART Driver with Circular FIFO Ring Buffer Echo
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT6 (Week 5 - 17/08/2026) - Task B
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Interrupt Architecture:
 *   - UART Interrupt Vector 4 (Address 0x0023)
 *   - Reception: Shared vector checks RI flag -> Enqueues into RX ring buffer -> RI = 0
 *   - Transmission: Shared vector checks TI flag -> Sets tx_ready flag -> TI = 0
 *   - Main Superloop: Dequeues received bytes from FIFO and echoes back to PC terminal
 * ==============================================================================
 */

#include "ring_buffer.h"

#if defined(__GNUC__)
  /* GCC syntax verification harness */
  static volatile unsigned char TMOD;
  static volatile unsigned char SCON;
  static volatile unsigned char PCON;
  static volatile unsigned char TH1;
  static volatile unsigned char TL1;
  static volatile unsigned char SBUF;
  static volatile unsigned char RI;
  static volatile unsigned char TI;
  #define TR1 1
  #define ES 1
  #define EA 1
#elif defined(__SDCC)
  #include <8052.h>
#else
  #include <reg51.h>
#endif

static ring_buffer_t rx_fifo;
static volatile unsigned char tx_busy = 0;

#if !defined(__GNUC__)
/**
 * @brief  UART Combined Interrupt Service Routine (Vector 4, address 0x0023).
 *         Handles both byte reception (RI) and transmission complete (TI).
 */
void uart_isr(void) interrupt 4
{
    unsigned char incoming_byte;

    if (RI)
    {
        incoming_byte = SBUF;
        RI = 0; /* Clear hardware flag */

        /* Push byte into circular ring buffer */
        ring_buffer_put(&rx_fifo, incoming_byte);
    }

    if (TI)
    {
        TI = 0; /* Clear hardware flag */
        tx_busy = 0; /* Transmitter is now ready for next byte */
    }
}
#endif

/**
 * @brief  Configures 8051 UART peripheral with interrupt support.
 */
static void uart_interrupt_init(void)
{
    ring_buffer_init(&rx_fifo);
    tx_busy = 0;

    #if !defined(__GNUC__)
    /* Timer 1 Mode 2: 8-bit auto-reload */
    TMOD = (TMOD & 0x0F) | 0x20;

    /* SMOD = 0, 9600 Baud reload for 11.0592 MHz crystal */
    PCON &= 0x7F;
    TH1   = 0xFD;
    TL1   = 0xFD;

    /* Mode 1 8-bit UART, Receive Enabled */
    SCON  = 0x50;

    /* Enable Serial Interrupt (ES) and Global Interrupts (EA) */
    ES    = 1;
    EA    = 1;

    /* Start Baud Timer 1 */
    TR1   = 1;
    #endif
}

/**
 * @brief  Transmits a byte using interrupt-synchronized handshake.
 */
static void uart_send_byte(unsigned char byte)
{
    #if !defined(__GNUC__)
    while (tx_busy); /* Wait for preceding transmission to complete */
    tx_busy = 1;
    SBUF = byte;
    #else
    (void)byte;
    #endif
}

/**
 * @brief  Transmits a string of characters.
 */
static void uart_send_string(const char *str)
{
    while (*str)
    {
        uart_send_byte(*str);
        str++;
    }
}

/**
 * @brief  Application entry point.
 */
void main(void)
{
    unsigned char echo_char;

    uart_interrupt_init();

    /* Transmit welcome banner upon connection */
    uart_send_string("\r\n=========================================\r\n");
    uart_send_string(" 8051 UART ECHO CONSOLE READY (9600 8-N-1)\r\n");
    uart_send_string("=========================================\r\n");

    while (1)
    {
        /* Check if circular FIFO has received data */
        if (ring_buffer_get(&rx_fifo, &echo_char))
        {
            /* Echo character back to host terminal */
            uart_send_byte(echo_char);

            /* If carriage return received, issue additional line feed */
            if (echo_char == '\r')
            {
                uart_send_byte('\n');
            }
        }
    }
}
