/**
 * ==============================================================================
 * @file           : ring_buffer.h
 * @brief          : Lock-Free Circular FIFO Ring Buffer Header
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT6 (Week 5 - 17/08/2026) - Task B
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#define RING_BUFFER_SIZE 32  /**< Buffer depth: power of 2 for fast bitwise masking */
#define RING_BUFFER_MASK (RING_BUFFER_SIZE - 1)

typedef struct
{
    unsigned char storage[RING_BUFFER_SIZE];
    volatile unsigned char head_idx;
    volatile unsigned char tail_idx;
} ring_buffer_t;

/**
 * @brief  Initializes the circular buffer structure.
 */
void ring_buffer_init(ring_buffer_t *rb);

/**
 * @brief  Enqueues a single byte into the ring buffer.
 * @return 1 on success, 0 if buffer was full (overflow prevented).
 */
unsigned char ring_buffer_put(ring_buffer_t *rb, unsigned char data_byte);

/**
 * @brief  Dequeues a single byte from the ring buffer.
 * @return 1 if byte extracted, 0 if buffer was empty.
 */
unsigned char ring_buffer_get(ring_buffer_t *rb, unsigned char *out_byte);

/**
 * @brief  Checks whether the buffer contains readable data.
 */
unsigned char ring_buffer_is_empty(const ring_buffer_t *rb);

#endif /* RING_BUFFER_H */
