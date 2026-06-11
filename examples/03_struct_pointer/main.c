#include <stdio.h>
#include "gps/gps_types.h"

static void print_fix_by_value(gps_fix_t fix)
{
    fix.satellites = 99;
    printf("[傳值] 函式內 sats=%u\n", fix.satellites);
}

static void print_fix_by_pointer(gps_fix_t *fix)
{
    fix->satellites = 99;
    printf("[傳指標] 函式內 sats=%u (透過 -> 存取)\n", fix->satellites);
}

int main(void)
{
    gps_fix_t fix = {
        .latitude   = 25.0334,
        .longitude  = 121.5645,
        .altitude_m = 12.5,
        .satellites = 8,
        .valid      = true
    };

    printf("sizeof(gps_fix_t) = %zu bytes\n", sizeof(fix));

    print_fix_by_value(fix);
    printf("呼叫後 main 中 sats=%u (沒變)\n", fix.satellites);

    print_fix_by_pointer(&fix);
    printf("呼叫後 main 中 sats=%u (被改了)\n", fix.satellites);

    return 0;
}
