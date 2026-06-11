#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t       *buffer;
    size_t         capacity;
    volatile size_t head;
    volatile size_t tail;
} ring_buffer_t;

void   ring_buffer_init(ring_buffer_t *rb, uint8_t *storage, size_t capacity);
bool   ring_buffer_push(ring_buffer_t *rb, uint8_t byte);
bool   ring_buffer_pop(ring_buffer_t *rb, uint8_t *out);
size_t ring_buffer_count(const ring_buffer_t *rb);
bool   ring_buffer_is_empty(const ring_buffer_t *rb);
bool   ring_buffer_is_full(const ring_buffer_t *rb);
void   ring_buffer_reset(ring_buffer_t *rb);

#endif
