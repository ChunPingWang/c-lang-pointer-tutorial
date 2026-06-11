#include <stdio.h>
#include "gps/nmea_parser.h"

static void log_handler(const gps_fix_t *fix, void *user_data)
{
    const char *tag = (const char *)user_data;
    printf("[%s] lat=%.4f lon=%.4f sats=%u\n",
           tag, fix->latitude, fix->longitude, fix->satellites);
}

static void counter_handler(const gps_fix_t *fix, void *user_data)
{
    (void)fix;
    int *counter = (int *)user_data;
    (*counter)++;
}

int main(void)
{
    const char *stream =
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n"
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";

    printf("-- handler A: log to stdout --\n");
    nmea_parser_t p1;
    nmea_parser_init(&p1, log_handler, (void *)"demo");
    for (const char *c = stream; *c; ++c) nmea_parser_feed_byte(&p1, *c);

    printf("-- handler B: just count --\n");
    int count = 0;
    nmea_parser_t p2;
    nmea_parser_init(&p2, counter_handler, &count);
    for (const char *c = stream; *c; ++c) nmea_parser_feed_byte(&p2, *c);
    printf("count = %d\n", count);

    return 0;
}
