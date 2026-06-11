#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "gps/gps_types.h"

#define NMEA_MAX_SENTENCE_LEN 96
#define NMEA_MAX_FIELDS       20

typedef enum {
    NMEA_OK = 0,
    NMEA_ERR_TOO_SHORT,
    NMEA_ERR_NO_DOLLAR,
    NMEA_ERR_NO_CHECKSUM,
    NMEA_ERR_BAD_CHECKSUM,
    NMEA_ERR_UNSUPPORTED,
    NMEA_ERR_BAD_FIELD
} nmea_status_t;

typedef void (*nmea_fix_cb_t)(const gps_fix_t *fix, void *user_data);

typedef struct {
    char           sentence_buf[NMEA_MAX_SENTENCE_LEN];
    size_t         length;
    bool           collecting;
    gps_fix_t      latest_fix;
    nmea_fix_cb_t  on_fix;
    void          *user_data;
} nmea_parser_t;

void          nmea_parser_init(nmea_parser_t *p, nmea_fix_cb_t cb, void *user_data);
nmea_status_t nmea_parser_feed_byte(nmea_parser_t *p, char byte);
nmea_status_t nmea_parser_parse_sentence(const char *sentence, gps_fix_t *out_fix);
uint8_t       nmea_compute_checksum(const char *body, size_t len);
bool          nmea_verify_checksum(const char *sentence);

#endif
