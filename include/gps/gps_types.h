#ifndef GPS_TYPES_H
#define GPS_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define GPS_TALKER_LEN   2
#define GPS_SENTENCE_LEN 6

typedef enum {
    GPS_SENTENCE_UNKNOWN = 0,
    GPS_SENTENCE_RMC,
    GPS_SENTENCE_GGA
} gps_sentence_id_t;

typedef enum {
    GPS_FIX_NONE   = 0,
    GPS_FIX_GPS    = 1,
    GPS_FIX_DGPS   = 2
} gps_fix_quality_t;

typedef struct {
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint16_t millisecond;
} gps_time_t;

typedef struct {
    double   latitude;
    double   longitude;
    double   altitude_m;
    double   speed_knots;
    double   course_deg;
    gps_time_t time;
    uint8_t  satellites;
    gps_fix_quality_t fix_quality;
    bool     valid;
} gps_fix_t;

#endif
