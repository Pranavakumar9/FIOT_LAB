/**
 * ==============================================================================
 * @file           : ring_buffer.c
 * @brief          : Lock-Free Circular FIFO Ring Buffer Implementation
 * @course         : 25CS2741 Fundamentals of Internet of Things + Lab
 * @evaluation     : CAT6 (Week 5 - 17/08/2026) - Task B
 * @target         : Atmel AT89S52 @ 11.0592 MHz
 * @author         : Pranavakumar (Shiv Nadar University - Chennai)
 * ==============================================================================
 */

#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb)
{
    rb->head_idx = 0;
    rb->tail_idx = 0;
}

unsigned char ring_buffer_put(ring_buffer_t *rb, unsigned char data_byte)
{
    unsigned char next_head = (rb->head_idx + 1) & RING_BUFFER_MASK;

    /* Drop byte if buffer is full to prevent pointer collision */
    if (next_head == rb->tail_idx)
    {
        return 0; /* Overflow condition */
    }

    rb->storage[rb->head_idx] = data_byte;
    rb->head_idx = next_head;
    return 1;
}

unsigned char ring_buffer_get(ring_buffer_t *rb, unsigned char *out_byte)
{
    if (rb->head_idx == rb->tail_idx)
    {
        return 0; /* Buffer is empty */
    }

    *out_byte = rb->storage[rb->tail_idx];
    rb->tail_idx = (rb->tail_idx + 1) & RING_BUFFER_MASK;
    return 1;
}

unsigned char ring_buffer_is_empty(const ring_buffer_t *rb)
{
    return (rb->head_idx == rb->tail_idx);
}
