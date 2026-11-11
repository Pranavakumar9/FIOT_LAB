/**
 * ==============================================================================
 * @file           : main.c
 * @brief          : Stepper Motor Speed & Direction Controller via ULN2003 Driver
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT5 (Week 4 - 10/08/2026) - Task 3
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 * Interface Configuration:
 *   - P2.0 (Pin 21) -> ULN2003 IN1 -> Motor Coil A (Blue)
 *   - P2.1 (Pin 22) -> ULN2003 IN2 -> Motor Coil B (Pink)
 *   - P2.2 (Pin 23) -> ULN2003 IN3 -> Motor Coil C (Yellow)
 *   - P2.3 (Pin 24) -> ULN2003 IN4 -> Motor Coil D (Orange)
 *   - ULN2003 COM   -> Motor Red Lead (+5.0V External DC)
 *
 * Single-Phase Wave Drive Excitation Sequence:
 *   Step 0: 0x01 (0001) -> Coil A energized
 *   Step 1: 0x02 (0010) -> Coil B energized
 *   Step 2: 0x04 (0100) -> Coil C energized
 *   Step 3: 0x08 (1000) -> Coil D energized
 *
 * Speed Modes:
 *   - SPEED_SLOW:   100 ms per step (10 steps/sec)
 *   - SPEED_MEDIUM: 50 ms per step  (20 steps/sec)
 *   - SPEED_FAST:   10 ms per step  (100 steps/sec)
 * ==============================================================================
 */

#if defined(__GNUC__)
  /* GCC syntax verification harness */
  static volatile unsigned char TMOD;
  static volatile unsigned char TH0;
  static volatile unsigned char TL0;
  static volatile unsigned char P2;
  #define ET0 1
  #define EA 1
  #define TR0 1
  #define ROM_CODE
#elif defined(__SDCC)
  #include <8052.h>
  #define ROM_CODE __code
#else
  #include <reg51.h>
  #define ROM_CODE code
#endif

typedef enum
{
    SPEED_SLOW   = 100,  /**< 100 ms dwell between step transitions */
    SPEED_MEDIUM = 50,   /**< 50 ms dwell between step transitions */
    SPEED_FAST   = 10    /**< 10 ms dwell between step transitions */
} motor_speed_t;

typedef enum
{
    DIR_CW  = 0,
    DIR_CCW = 1
} motor_dir_t;

/* 4-Step Single-Phase Wave Drive Sequence Table */
static const unsigned char ROM_CODE WAVE_DRIVE_STEPS[4] = {
    0x08,  /* Step 0: 1000 -> Coil 4 */
    0x04,  /* Step 1: 0100 -> Coil 3 */
    0x02,  /* Step 2: 0010 -> Coil 2 */
    0x01   /* Step 3: 0001 -> Coil 1 */
};

static volatile unsigned int g_tick_counter = 0;

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

#if !defined(__GNUC__)
/**
 * @brief  Timer 0 1.0 ms Interrupt Service Routine
 */
void timer0_isr(void) interrupt 1
{
    TH0 = 0xFC;
    TL0 = 0x66;
    g_tick_counter++;
}
#endif

/**
 * @brief  Hardware initialization for Timer 0 and Motor Port.
 */
static void motor_hardware_init(void)
{
    #if !defined(__GNUC__)
    /* Timer 0 Mode 1 (16-bit) */
    TMOD = (TMOD & 0xF0) | 0x01;
    TH0 = 0xFC;
    TL0 = 0x66;

    ET0 = 1; /* Enable Timer 0 Interrupt */
    EA  = 1; /* Enable Global Interrupts */
    TR0 = 1; /* Start Timer 0 */
    #endif

    /* De-energize all coils */
    P2 = P2 & 0xF0;
}

/**
 * @brief  Main execution superloop.
 */
void main(void)
{
    unsigned char current_step = 0;
    unsigned int last_step_time = 0;
    motor_speed_t active_speed = SPEED_MEDIUM;
    motor_dir_t   active_direction = DIR_CW;
    unsigned int now;

    motor_hardware_init();

    while (1)
    {
        now = get_ticks();

        if ((unsigned int)(now - last_step_time) >= (unsigned int)active_speed)
        {
            last_step_time = now;

            /* Output coil excitation pattern to ULN2003 inputs */
            P2 = (P2 & 0xF0) | WAVE_DRIVE_STEPS[current_step];

            /* Advance step index based on direction */
            if (active_direction == DIR_CW)
            {
                current_step = (current_step + 1) & 0x03;
            }
            else
            {
                current_step = (current_step == 0) ? 3 : (current_step - 1);
            }
        }
    }
}
