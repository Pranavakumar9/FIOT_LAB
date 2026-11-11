/**
 * ==============================================================================
 * @file           : pattern_fsm.h
 * @brief          : Finite State Machine Header for Multi-Mode LED Pattern Generator
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT3 (Week 2 - 27/07/2026)
 * @target         : Atmel AT89S52
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#ifndef PATTERN_FSM_H
#define PATTERN_FSM_H

/**
 * @brief System operating modes
 */
typedef enum
{
    SYS_MODE_OFF        = 0,  /**< Initial state: all LEDs extinguished */
    SYS_MODE_RUNNING    = 1,  /**< Mode 1: Chaser pattern (LED1 -> LED2 -> LED3 -> LED4) */
    SYS_MODE_BINARY     = 2,  /**< Mode 2: 4-bit binary up-counter (0000 to 1111) */
    SYS_MODE_ALTERNATE  = 3,  /**< Mode 3: Alternating dual-phase pattern (1010 <-> 0101) */
    SYS_MODE_FLASH_ALL  = 4   /**< Mode 4: Synchronous strobe of all 4 LEDs (1111 <-> 0000) */
} sys_mode_t;

/**
 * @brief  Initializes the LED GPIO port and resets the internal state machine.
 */
void fsm_init(void);

/**
 * @brief  Advances the system to the next sequential operational mode.
 */
void fsm_advance_mode(void);

/**
 * @brief  Executes periodic pattern updates based on non-blocking timer ticks.
 * @param  current_ms: Current elapsed timebase in milliseconds.
 */
void fsm_update_cadence(unsigned int current_ms);

/**
 * @brief  Returns the current active operational mode.
 */
sys_mode_t fsm_get_active_mode(void);

#endif /* PATTERN_FSM_H */
