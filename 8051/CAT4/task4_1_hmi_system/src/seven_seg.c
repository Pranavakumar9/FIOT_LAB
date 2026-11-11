/**
 * ==============================================================================
 * @file           : seven_seg.c
 * @brief          : Seven-Segment Display Driver Implementation
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT4 (Week 3 - 03/08/2026)
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#include "seven_seg.h"

#if defined(__GNUC__)
  static volatile unsigned char P0;
  #define ROM_CODE
#elif defined(__SDCC)
  #include <8052.h>
  #define ROM_CODE __code
#else
  #include <reg51.h>
  #define ROM_CODE code
#endif

/*
 * Common-Anode Seven-Segment Encoding Table (Segments a,b,c,d,e,f,g,dp)
 * Active-LOW: bit = 0 illuminates segment, bit = 1 turns off segment.
 * P0.0=a, P0.1=b, P0.2=c, P0.3=d, P0.4=e, P0.5=f, P0.6=g, P0.7=dp
 */
static const unsigned char ROM_CODE SEVEN_SEG_TABLE[10] = {
    0xC0,  /* 0: a,b,c,d,e,f ON */
    0xF9,  /* 1: b,c ON */
    0xA4,  /* 2: a,b,d,e,g ON */
    0xB0,  /* 3: a,b,c,d,g ON */
    0x99,  /* 4: b,c,f,g ON */
    0x92,  /* 5: a,c,d,f,g ON */
    0x82,  /* 6: a,c,d,e,f,g ON */
    0xF8,  /* 7: a,b,c ON */
    0x80,  /* 8: All segments a-g ON */
    0x90   /* 9: a,b,c,d,f,g ON */
};

void seven_seg_init(void)
{
    /* Extinguish display (all segments high) */
    P0 = 0xFF;
}

void display_digit(unsigned char value)
{
    if (value <= 9)
    {
        P0 = SEVEN_SEG_TABLE[value];
    }
}

void display_clear(void)
{
    P0 = 0xFF;
}
