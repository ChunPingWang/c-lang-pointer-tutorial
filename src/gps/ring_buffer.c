#include "gps/ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb, uint8_t *storage, size_t capacity)
{
    rb->buffer   = storage;
    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
}

void ring_buffer_reset(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

bool ring_buffer_is_empty(const ring_buffer_t *rb)
{
    return rb->head == rb->tail;
}

bool ring_buffer_is_full(const ring_buffer_t *rb)
{
    return ((rb->head + 1) % rb->capacity) == rb->tail;
}

size_t ring_buffer_count(const ring_buffer_t *rb)
{
    if (rb->head >= rb->tail) {
        return rb->head - rb->tail;
    }
    return rb->capacity - rb->tail + rb->head;
}

bool ring_buffer_push(ring_buffer_t *rb, uint8_t byte)
{
    size_t next = (rb->head + 1) % rb->capacity;
    if (next == rb->tail) {
        return false;
    }
    rb->buffer[rb->head] = byte;
    rb->head = next;
    return true;
}

bool ring_buffer_pop(ring_buffer_t *rb, uint8_t *out)
{
    if (rb->head == rb->tail) {
        return false;
    }
    *out = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    return true;
}
