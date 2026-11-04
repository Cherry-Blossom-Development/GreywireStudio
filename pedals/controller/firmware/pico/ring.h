/* Byte ring buffer (one writer, one reader). Size must be a power of two. */
#ifndef RING_H
#define RING_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *buf;
    uint32_t mask;
    volatile uint32_t head, tail;
} ring;

#define RING(name, size) static uint8_t name##_buf[size]; static ring name = {name##_buf, (size) - 1, 0, 0}

static inline bool ring_empty(const ring *r) { return r->head == r->tail; }
static inline uint32_t ring_used(const ring *r) { return r->head - r->tail; }

static inline bool ring_put(ring *r, uint8_t b)
{
    if (r->head - r->tail > r->mask)
        return false;                        /* full: byte dropped */
    r->buf[r->head & r->mask] = b;
    r->head++;
    return true;
}

/* whole message or nothing, so a full buffer never cuts a message in half */
static inline bool ring_put_msg(ring *r, const uint8_t *m, int n)
{
    if ((uint32_t)n > r->mask + 1 - ring_used(r))
        return false;
    for (int i = 0; i < n; i++)
        ring_put(r, m[i]);
    return true;
}

static inline int ring_get(ring *r)
{
    if (ring_empty(r))
        return -1;
    uint8_t b = r->buf[r->tail & r->mask];
    r->tail++;
    return b;
}

#endif
