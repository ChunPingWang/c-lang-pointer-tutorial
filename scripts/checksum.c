/*
 * 輔助工具: 計算 NMEA sentence checksum
 * 用法:   ./checksum "GPRMC,123519,A,4807.038,N,..."
 *
 * 注意傳入時不要含 $ 和 *XX，這個工具只計算兩者之間的 XOR。
 */

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <body without $ and *XX>\n", argv[0]);
        return 1;
    }
    unsigned char c = 0;
    for (size_t i = 0; i < strlen(argv[1]); ++i) {
        c ^= (unsigned char)argv[1][i];
    }
    printf("*%02X\n", c);
    return 0;
}
