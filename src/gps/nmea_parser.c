#include "gps/nmea_parser.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

uint8_t nmea_compute_checksum(const char *body, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum ^= (uint8_t)body[i];
    }
    return sum;
}

bool nmea_verify_checksum(const char *sentence)
{
    if (sentence == NULL || sentence[0] != '$') {
        return false;
    }
    const char *star = strchr(sentence, '*');
    if (star == NULL || star[1] == '\0' || star[2] == '\0') {
        return false;
    }
    int hi = hex_value(star[1]);
    int lo = hex_value(star[2]);
    if (hi < 0 || lo < 0) {
        return false;
    }
    uint8_t expected = (uint8_t)((hi << 4) | lo);
    size_t  body_len = (size_t)(star - (sentence + 1));
    return nmea_compute_checksum(sentence + 1, body_len) == expected;
}

static size_t split_fields(char *sentence, char **fields, size_t max_fields)
{
    size_t n = 0;
    char  *cursor = sentence;
    fields[n++] = cursor;
    while (*cursor != '\0' && *cursor != '*' && n < max_fields) {
        if (*cursor == ',') {
            *cursor = '\0';
            fields[n++] = cursor + 1;
        }
        cursor++;
    }
    if (*cursor == '*') {
        *cursor = '\0';
    }
    return n;
}

static double parse_coord(const char *raw, char hemisphere)
{
    if (raw == NULL || *raw == '\0') return 0.0;

    double full = atof(raw);
    double degrees;
    double minutes = modf(full / 100.0, &degrees) * 100.0;
    double result  = degrees + minutes / 60.0;
    if (hemisphere == 'S' || hemisphere == 'W') {
        result = -result;
    }
    return result;
}

static void parse_time(const char *raw, gps_time_t *out)
{
    if (raw == NULL || strlen(raw) < 6) {
        out->hour = out->minute = out->second = 0;
        out->millisecond = 0;
        return;
    }
    char buf[3] = {0};
    buf[0] = raw[0]; buf[1] = raw[1]; out->hour   = (uint8_t)atoi(buf);
    buf[0] = raw[2]; buf[1] = raw[3]; out->minute = (uint8_t)atoi(buf);
    buf[0] = raw[4]; buf[1] = raw[5]; out->second = (uint8_t)atoi(buf);
    out->millisecond = 0;
    if (raw[6] == '.') {
        out->millisecond = (uint16_t)(atof(raw + 6) * 1000.0);
    }
}

static nmea_status_t parse_rmc(char **fields, size_t n, gps_fix_t *out)
{
    if (n < 10) return NMEA_ERR_BAD_FIELD;
    parse_time(fields[1], &out->time);
    out->valid       = (fields[2][0] == 'A');
    out->latitude    = parse_coord(fields[3], fields[4][0]);
    out->longitude   = parse_coord(fields[5], fields[6][0]);
    out->speed_knots = atof(fields[7]);
    out->course_deg  = atof(fields[8]);
    return NMEA_OK;
}

static nmea_status_t parse_gga(char **fields, size_t n, gps_fix_t *out)
{
    if (n < 10) return NMEA_ERR_BAD_FIELD;
    parse_time(fields[1], &out->time);
    out->latitude    = parse_coord(fields[2], fields[3][0]);
    out->longitude   = parse_coord(fields[4], fields[5][0]);
    out->fix_quality = (gps_fix_quality_t)atoi(fields[6]);
    out->satellites  = (uint8_t)atoi(fields[7]);
    out->altitude_m  = atof(fields[9]);
    out->valid       = (out->fix_quality != GPS_FIX_NONE);
    return NMEA_OK;
}

static gps_sentence_id_t identify_sentence(const char *type)
{
    if (strlen(type) < 5) return GPS_SENTENCE_UNKNOWN;
    if (strcmp(type + 2, "RMC") == 0) return GPS_SENTENCE_RMC;
    if (strcmp(type + 2, "GGA") == 0) return GPS_SENTENCE_GGA;
    return GPS_SENTENCE_UNKNOWN;
}

nmea_status_t nmea_parser_parse_sentence(const char *sentence, gps_fix_t *out_fix)
{
    if (sentence == NULL || out_fix == NULL) return NMEA_ERR_BAD_FIELD;
    if (sentence[0] != '$') return NMEA_ERR_NO_DOLLAR;
    if (strchr(sentence, '*') == NULL) return NMEA_ERR_NO_CHECKSUM;
    if (!nmea_verify_checksum(sentence)) return NMEA_ERR_BAD_CHECKSUM;

    char working[NMEA_MAX_SENTENCE_LEN];
    strncpy(working, sentence + 1, sizeof(working) - 1);
    working[sizeof(working) - 1] = '\0';

    char  *fields[NMEA_MAX_FIELDS] = {0};
    size_t n = split_fields(working, fields, NMEA_MAX_FIELDS);
    if (n < 1) return NMEA_ERR_BAD_FIELD;

    switch (identify_sentence(fields[0])) {
        case GPS_SENTENCE_RMC: return parse_rmc(fields, n, out_fix);
        case GPS_SENTENCE_GGA: return parse_gga(fields, n, out_fix);
        default:               return NMEA_ERR_UNSUPPORTED;
    }
}

void nmea_parser_init(nmea_parser_t *p, nmea_fix_cb_t cb, void *user_data)
{
    memset(p, 0, sizeof(*p));
    p->on_fix    = cb;
    p->user_data = user_data;
}

nmea_status_t nmea_parser_feed_byte(nmea_parser_t *p, char byte)
{
    if (byte == '$') {
        p->collecting = true;
        p->length = 0;
        p->sentence_buf[p->length++] = byte;
        return NMEA_OK;
    }
    if (!p->collecting) {
        return NMEA_OK;
    }
    if (byte == '\r' || byte == '\n') {
        if (p->length == 0) return NMEA_OK;
        p->sentence_buf[p->length] = '\0';
        p->collecting = false;
        nmea_status_t st = nmea_parser_parse_sentence(p->sentence_buf, &p->latest_fix);
        if (st == NMEA_OK && p->on_fix != NULL) {
            p->on_fix(&p->latest_fix, p->user_data);
        }
        p->length = 0;
        return st;
    }
    if (p->length >= NMEA_MAX_SENTENCE_LEN - 1) {
        p->collecting = false;
        p->length = 0;
        return NMEA_ERR_TOO_SHORT;
    }
    p->sentence_buf[p->length++] = byte;
    return NMEA_OK;
}
