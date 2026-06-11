#include "test_helpers.h"
#include "gps/nmea_parser.h"
#include <string.h>

static int g_callback_count = 0;
static gps_fix_t g_last_fix;

static void on_fix(const gps_fix_t *fix, void *user_data)
{
    (void)user_data;
    g_callback_count++;
    g_last_fix = *fix;
}

int main(void)
{
    printf("Testing nmea_parser...\n");

    TEST_CASE("checksum xor known sentence");
    {
        const char *body = "GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W";
        EXPECT_EQ_INT(nmea_compute_checksum(body, strlen(body)), 0x6A);
    }

    TEST_CASE("verify_checksum accepts valid sentence");
    EXPECT(nmea_verify_checksum(
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"));

    TEST_CASE("verify_checksum rejects corrupted sentence");
    EXPECT(!nmea_verify_checksum(
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*00"));

    TEST_CASE("parse RMC fields");
    {
        gps_fix_t fix = {0};
        nmea_status_t st = nmea_parser_parse_sentence(
            "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A",
            &fix);
        EXPECT_EQ_INT(st, NMEA_OK);
        EXPECT_EQ_INT(fix.time.hour, 12);
        EXPECT_EQ_INT(fix.time.minute, 35);
        EXPECT_EQ_INT(fix.time.second, 19);
        EXPECT(fix.valid);
        EXPECT_NEAR(fix.latitude,  48.1173,  0.001);
        EXPECT_NEAR(fix.longitude, 11.51667, 0.001);
        EXPECT_NEAR(fix.speed_knots, 22.4, 0.01);
    }

    TEST_CASE("parse GGA fields");
    {
        gps_fix_t fix = {0};
        nmea_status_t st = nmea_parser_parse_sentence(
            "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
            &fix);
        EXPECT_EQ_INT(st, NMEA_OK);
        EXPECT_EQ_INT(fix.satellites, 8);
        EXPECT_NEAR(fix.altitude_m, 545.4, 0.01);
        EXPECT_EQ_INT(fix.fix_quality, GPS_FIX_GPS);
    }

    TEST_CASE("southern / western hemispheres negated");
    {
        gps_fix_t fix = {0};
        nmea_parser_parse_sentence(
            "$GPGGA,123519,4807.038,S,01131.000,W,1,08,0.9,545.4,M,46.9,M,,*48",
            &fix);
        EXPECT(fix.latitude  < 0.0);
        EXPECT(fix.longitude < 0.0);
    }

    TEST_CASE("missing $ rejected");
    {
        gps_fix_t fix = {0};
        EXPECT_EQ_INT(nmea_parser_parse_sentence("GPRMC,...", &fix),
                      NMEA_ERR_NO_DOLLAR);
    }

    TEST_CASE("streaming feed invokes callback");
    {
        nmea_parser_t p;
        g_callback_count = 0;
        nmea_parser_init(&p, on_fix, NULL);
        const char *stream =
            "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n"
            "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
        for (const char *c = stream; *c; ++c) {
            nmea_parser_feed_byte(&p, *c);
        }
        EXPECT_EQ_INT(g_callback_count, 2);
        EXPECT_EQ_INT(g_last_fix.satellites, 8);
    }

    TEST_REPORT();
}
