#include <stdio.h>
#include <string.h>

static size_t split_inplace(char *line, char **fields, size_t max_fields)
{
    size_t n = 0;
    char  *cursor = line;
    fields[n++] = cursor;
    while (*cursor && n < max_fields) {
        if (*cursor == ',') {
            *cursor = '\0';
            fields[n++] = cursor + 1;
        }
        cursor++;
    }
    return n;
}

int main(void)
{
    char  sentence[] =
        "GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W";
    char *fields[16] = {0};

    size_t n = split_inplace(sentence, fields, 16);

    printf("拆出 %zu 個欄位:\n", n);
    for (size_t i = 0; i < n; ++i) {
        printf("  fields[%zu] = \"%s\"  (位址 %p)\n",
               i, fields[i], (void *)fields[i]);
    }

    printf("\nfields 自己是 char**, sizeof(fields[0]) = %zu (一個指標的大小)\n",
           sizeof(fields[0]));
    return 0;
}
