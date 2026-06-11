#include <stdio.h>
#include <string.h>

static size_t count_commas(const char *s)
{
    size_t n = 0;
    while (*s) {
        if (*s++ == ',') n++;
    }
    return n;
}

int main(void)
{
    const char  sentence[] =
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";

    printf("陣列名 sentence 退化為指標時: %p\n", (void *)sentence);
    printf("sentence + 1 指向 '%c'\n", *(sentence + 1));
    printf("sentence[3] 等價於 *(sentence + 3) = '%c'\n", sentence[3]);

    printf("欄位數 (逗號數+1) = %zu\n", count_commas(sentence) + 1);

    const char *cursor = sentence;
    const char *star   = strchr(cursor, '*');
    printf("checksum 起始於相對偏移 %ld 處\n", star - sentence);

    return 0;
}
