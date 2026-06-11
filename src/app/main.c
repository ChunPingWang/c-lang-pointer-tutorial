#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gps/nmea_parser.h"
#include "gps/ring_buffer.h"
#include "hal/uart_mock.h"

#define RX_BUFFER_SIZE 256

static const char DEMO_STREAM[] =
    "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n"
    "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"
    "$GPRMC,205720,A,2503.6319,N,12126.5862,E,000.0,000.0,120625,,,A*76\r\n"
    "$GPGGA,205720,2503.6319,N,12126.5862,E,1,10,0.8,42.0,M,15.0,M,,*7D\r\n";

typedef struct {
    int fix_count;
} app_state_t;

static void on_new_fix(const gps_fix_t *fix, void *user_data)
{
    app_state_t *state = (app_state_t *)user_data;
    state->fix_count++;

    printf("[fix #%d] time=%02u:%02u:%02u  lat=%.6f  lon=%.6f  "
           "sats=%u  alt=%.1fm  speed=%.1fkn  valid=%s\n",
           state->fix_count,
           fix->time.hour, fix->time.minute, fix->time.second,
           fix->latitude, fix->longitude,
           fix->satellites, fix->altitude_m,
           fix->speed_knots,
           fix->valid ? "yes" : "no");
}

/*
 * 讀檔到記憶體, 回傳緩衝區指標 (呼叫端負責 free)。
 * 失敗時印錯誤訊息並回 NULL。
 *
 * 教學重點:
 *   - FILE *  : 不透明指標, 由 stdio 管理
 *   - malloc  : 動態配置, 必須 free
 *   - size_t *: out parameter (透過指標回傳第二個值)
 */
static uint8_t *read_whole_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        perror(path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz < 0) {
        fclose(f);
        return NULL;
    }

    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (buf == NULL) {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, (size_t)sz, f);
    fclose(f);

    if (read_bytes != (size_t)sz) {
        free(buf);
        return NULL;
    }

    *out_size = read_bytes;
    return buf;
}

int main(int argc, char *argv[])
{
    uint8_t        storage[RX_BUFFER_SIZE];
    ring_buffer_t  rx;
    uart_mock_t    uart;
    nmea_parser_t  parser;
    app_state_t    state = {0};

    const uint8_t *source     = (const uint8_t *)DEMO_STREAM;
    size_t         source_len = sizeof(DEMO_STREAM) - 1;
    uint8_t       *heap_buf   = NULL;

    if (argc >= 2) {
        heap_buf = read_whole_file(argv[1], &source_len);
        if (heap_buf == NULL) {
            return 1;
        }
        source = heap_buf;
        printf("(reading NMEA from %s, %zu bytes)\n", argv[1], source_len);
    } else {
        printf("(no file given; using built-in demo stream. "
               "try: ./gps_demo data/sample_nmea.txt)\n");
    }

    ring_buffer_init(&rx, storage, sizeof(storage));
    uart_mock_init(&uart, &rx, source, source_len);
    nmea_parser_init(&parser, on_new_fix, &state);

    printf("=== GPS pipeline demo (host build) ===\n");

    while (!uart_mock_finished(&uart) || !ring_buffer_is_empty(&rx)) {
        uart_mock_pump(&uart, 16);
        uint8_t byte;
        while (ring_buffer_pop(&rx, &byte)) {
            nmea_parser_feed_byte(&parser, (char)byte);
        }
    }

    printf("=== total fixes parsed: %d ===\n", state.fix_count);

    free(heap_buf);
    return 0;
}
