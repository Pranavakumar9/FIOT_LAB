/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : Software PWM LED Fading and Variable Frequency Signal Generator
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT5 (Week 4 - 10/08/2026) - Task 2
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Engineering Principles:
 *   1. Software PWM LED Fading (P1.0):
 *      - Base PWM Frequency: 500 Hz (Period T = 2.0 ms)
 *      - Time slice: 200 us per step (10-step resolution: 0% to 100%)
 *      - Timer 0 Mode 2 (8-bit auto-reload): Reload = 256 - 184 = 0x48
 *      - Fading ramp adjusts duty cycle every 50 ms (0% -> 100% -> 0%)
 *   2. Signal Generation (P1.1 to DSO):
 *      - Modes: 100 Hz, 500 Hz, 1 kHz, 2 kHz, 5 kHz
 *      - Waveform Types: Square, Variable Duty, Pulse Train, Burst Packet
 * ==============================================================================
 */

#if defined(__GNUC__)
  /* GCC syntax verification harness */
  static volatile unsigned char TMOD;
  static volatile unsigned char TH0;
  static volatile unsigned char TL0;
  static volatile unsigned char PIN_PWM_LED;
  static volatile unsigned char PIN_SIGNAL_OUT;
  #define ET0 1
  #define EA 1
  #define TR0 1
#elif defined(__SDCC)
  #include <8052.h>
  #define PIN_PWM_LED P1_0
  #define PIN_SIGNAL_OUT P1_1
#else
  #include <reg51.h>
  sbit PIN_PWM_LED = P1^0;
  sbit PIN_SIGNAL_OUT = P1^1;
#endif

/* Operational Signal Modes */
typedef enum
{
    SIG_MODE_100HZ  = 1,
    SIG_MODE_500HZ  = 2,
    SIG_MODE_1KHZ   = 3,
    SIG_MODE_2KHZ   = 4,
    SIG_MODE_5KHZ   = 5
} sig_freq_mode_t;

typedef enum
{
    WAVE_SQUARE         = 1,
    WAVE_VARIABLE_DUTY  = 2,
    WAVE_PULSE_TRAIN    = 3,
    WAVE_BURST_PACKET   = 4
} wave_shape_t;

/* Global interrupt state counters */
static volatile unsigned char pwm_slice_counter = 0;
static volatile unsigned char current_duty_step = 0;   /* 0 to 10 (0% to 100%) */
static volatile unsigned int  fade_dwell_timer = 0;
static volatile unsigned char fade_direction_up = 1;

/* Signal generator state counters */
static volatile sig_freq_mode_t active_freq_mode = SIG_MODE_500HZ;
static volatile wave_shape_t    active_waveform  = WAVE_SQUARE;
static volatile unsigned int    sig_phase_counter = 0;
static volatile unsigned char   burst_counter = 0;

#if !defined(__GNUC__)
/**
 * @brief  Timer 0 ISR: Generates 200 us master time slices.
 *         At 11.0592 MHz, 184 machine cycles = 199.65 us (~200 us).
 *         Mode 2 automatically reloads TH0 (0x48) into TL0 on overflow.
 */
void timer0_slice_isr(void) interrupt 1
{
    unsigned int sig_period_slices;
    unsigned int sig_high_slices;

    /* -------------------------------------------------------------
     * SECTION 1: Software PWM Generation for LED Fading (P1.0)
     * ------------------------------------------------------------- */
    pwm_slice_counter++;
    if (pwm_slice_counter >= 10)
    {
        pwm_slice_counter = 0;
    }

    /* Active-LOW LED: Sinks current when pin = 0 (ON) */
    if (pwm_slice_counter < current_duty_step)
    {
        PIN_PWM_LED = 0; /* LED ON during active duty phase */
    }
    else
    {
        PIN_PWM_LED = 1; /* LED OFF during inactive duty phase */
    }

    /* -------------------------------------------------------------
     * SECTION 2: Dynamic Fading Ramp Control (Every 50 ms = 250 slices)
     * ------------------------------------------------------------- */
    fade_dwell_timer++;
    if (fade_dwell_timer >= 250)
    {
        fade_dwell_timer = 0;

        if (fade_direction_up)
        {
            if (++current_duty_step >= 10)
            {
                current_duty_step = 10;
                fade_direction_up = 0;
            }
        }
        else
        {
            if (current_duty_step > 0)
            {
                current_duty_step--;
            }
            else
            {
                current_duty_step = 0;
                fade_direction_up = 1;
            }
        }
    }

    /* -------------------------------------------------------------
     * SECTION 3: Signal Waveform Generation (P1.1)
     * ------------------------------------------------------------- */
    switch (active_freq_mode)
    {
        case SIG_MODE_100HZ: sig_period_slices = 50; break; /* 10 ms period = 50 * 200us */
        case SIG_MODE_500HZ: sig_period_slices = 10; break; /* 2 ms period = 10 * 200us */
        case SIG_MODE_1KHZ:  sig_period_slices = 5;  break; /* 1 ms period = 5 * 200us */
        case SIG_MODE_2KHZ:  sig_period_slices = 2;  break; /* 500 us period */
        case SIG_MODE_5KHZ:  sig_period_slices = 1;  break; /* 200 us period toggle */
        default:             sig_period_slices = 10; break;
    }

    sig_phase_counter++;
    if (sig_phase_counter >= sig_period_slices)
    {
        sig_phase_counter = 0;
    }

    /* Modulate output wave according to selected waveform type */
    switch (active_waveform)
    {
        case WAVE_SQUARE:
            /* Symmetrical 50% duty cycle */
            PIN_SIGNAL_OUT = (sig_phase_counter < (sig_period_slices / 2)) ? 1 : 0;
            break;

        case WAVE_VARIABLE_DUTY:
            /* 75% High, 25% Low */
            sig_high_slices = (sig_period_slices * 3) / 4;
            PIN_SIGNAL_OUT = (sig_phase_counter < sig_high_slices) ? 1 : 0;
            break;

        case WAVE_PULSE_TRAIN:
            /* Narrow 1-slice pulse */
            PIN_SIGNAL_OUT = (sig_phase_counter == 0) ? 1 : 0;
            break;

        case WAVE_BURST_PACKET:
            /* Burst of 4 pulses followed by 4 silent periods */
            if ((burst_counter & 0x08) == 0)
            {
                PIN_SIGNAL_OUT = (sig_phase_counter < (sig_period_slices / 2)) ? 1 : 0;
            }
            else
            {
                PIN_SIGNAL_OUT = 0;
            }
            if (sig_phase_counter == 0)
            {
                burst_counter++;
            }
            break;
    }
}
#endif

/**
 * @brief  System hardware configuration.
 */
static void configure_system(void)
{
    #if !defined(__GNUC__)
    /* Timer 0 Mode 2: 8-bit Auto-Reload (M1=1, M0=0) */
    TMOD = (TMOD & 0xF0) | 0x02;

    /* Preload = 256 - 184 = 0x48 (yields ~200 us period at 11.0592 MHz) */
    TH0 = 0x48;
    TL0 = 0x48;

    ET0 = 1; /* Enable Timer 0 Interrupt */
    EA  = 1; /* Enable Global Interrupts */
    TR0 = 1; /* Start Timer 0 */
    #endif

    PIN_PWM_LED = 1;
    PIN_SIGNAL_OUT = 0;
}

/**
 * @brief  Application entry point.
 */
void main(void)
{
    configure_system();

    while (1)
    {
        /* System operation is 100% interrupt driven and non-blocking */
    }
}
