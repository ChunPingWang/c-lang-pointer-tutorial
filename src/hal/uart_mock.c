#include "hal/uart_mock.h"

void uart_mock_init(uart_mock_t *u, ring_buffer_t *rx, const uint8_t *src, size_t len)
{
    u->rx_buffer  = rx;
    u->source     = src;
    u->source_len = len;
    u->cursor     = 0;
}

size_t uart_mock_pump(uart_mock_t *u, size_t bytes_to_emit)
{
    size_t emitted = 0;
    while (emitted < bytes_to_emit && u->cursor < u->source_len) {
        if (!ring_buffer_push(u->rx_buffer, u->source[u->cursor])) {
            break;
        }
        u->cursor++;
        emitted++;
    }
    return emitted;
}

bool uart_mock_finished(const uart_mock_t *u)
{
    return u->cursor >= u->source_len;
}
