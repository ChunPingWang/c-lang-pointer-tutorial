#ifndef UART_MOCK_H
#define UART_MOCK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "gps/ring_buffer.h"

typedef struct {
    ring_buffer_t *rx_buffer;
    const uint8_t *source;
    size_t         source_len;
    size_t         cursor;
} uart_mock_t;

void uart_mock_init(uart_mock_t *u, ring_buffer_t *rx, const uint8_t *src, size_t len);
size_t uart_mock_pump(uart_mock_t *u, size_t bytes_to_emit);
bool   uart_mock_finished(const uart_mock_t *u);

#endif
