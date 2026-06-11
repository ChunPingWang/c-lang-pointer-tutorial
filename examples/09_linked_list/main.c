#include <stdio.h>
#include "gps/track_log.h"
#include "gps/nmea_parser.h"

/* 把每筆 fix 加進 track_log 的 callback */
static void collect_to_log(const gps_fix_t *fix, void *user_data)
{
    track_log_t *log = (track_log_t *)user_data;
    if (!track_log_append(log, fix)) {
        fprintf(stderr, "track_log_append OOM!\n");
    }
}

static void print_node(const gps_fix_t *fix, size_t index, void *user_data)
{
    (void)user_data;
    printf("  [%zu] lat=%.4f lon=%.4f sats=%u valid=%s\n",
           index, fix->latitude, fix->longitude, fix->satellites,
           fix->valid ? "yes" : "no");
}

static bool not_valid(const gps_fix_t *fix, void *user_data)
{
    (void)user_data;
    return !fix->valid;
}

int main(void)
{
    /* 一段含一筆 invalid fix 的 NMEA 串流 (RMC 第 2 欄為 V 表示 void). */
    const char *stream =
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n"
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"
        "$GPRMC,205720,A,2503.6319,N,12126.5862,E,000.0,000.0,120625,,,A*76\r\n"
        "$GPGGA,205720,2503.6319,N,12126.5862,E,1,10,0.8,42.0,M,15.0,M,,*7D\r\n";

    track_log_t log;
    track_log_init(&log);

    nmea_parser_t p;
    nmea_parser_init(&p, collect_to_log, &log);
    for (const char *c = stream; *c; ++c) nmea_parser_feed_byte(&p, *c);

    printf("收集到的 fix (%zu 筆):\n", track_log_length(&log));
    track_log_foreach(&log, print_node, NULL);

    /* 注意: 範例中所有 fix 都 valid, 所以 remove_if 不會移除任何節點。
     * 但概念示範: 移除所有 invalid 的, 確保 track 只剩有效定位。 */
    size_t removed = track_log_remove_if(&log, not_valid, NULL);
    printf("\nremove_if(not_valid) 移除了 %zu 個節點, 現在 length=%zu\n",
           removed, track_log_length(&log));

    track_log_destroy(&log);
    printf("destroy 完成\n");
    return 0;
}
