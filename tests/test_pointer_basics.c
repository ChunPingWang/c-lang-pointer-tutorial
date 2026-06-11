#include "test_helpers.h"

static void swap_ints(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

static size_t count_chars(const char *s, char target)
{
    size_t n = 0;
    while (*s) {
        if (*s == target) n++;
        s++;
    }
    return n;
}

int main(void)
{
    printf("Testing pointer basics (concepts referenced from the tutorial)...\n");

    TEST_CASE("address-of and dereference are inverses");
    {
        int x = 42;
        int *p = &x;
        EXPECT_EQ_INT(*p, 42);
        *p = 100;
        EXPECT_EQ_INT(x, 100);
    }

    TEST_CASE("pointers let functions mutate caller state");
    {
        int a = 1, b = 2;
        swap_ints(&a, &b);
        EXPECT_EQ_INT(a, 2);
        EXPECT_EQ_INT(b, 1);
    }

    TEST_CASE("string is a pointer to first char");
    {
        const char *msg = "$GPRMC";
        EXPECT_EQ_INT(msg[0], '$');
        EXPECT_EQ_INT(*msg, '$');
        EXPECT_EQ_INT(*(msg + 3), 'R');
        EXPECT_EQ_INT(*(msg + 4), 'M');
    }

    TEST_CASE("counting commas in an NMEA sentence");
    {
        const char *sentence =
            "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
        EXPECT_EQ_INT(count_chars(sentence, ','), 11);
    }

    TEST_REPORT();
}
