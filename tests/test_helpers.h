#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int  test_passed = 0;
static int  test_failed = 0;
static const char *current_test = "";

#define TEST_CASE(name) \
    do { current_test = name; printf("  -> %s\n", name); } while(0)

#define EXPECT(cond) \
    do { \
        if (cond) { test_passed++; } \
        else { \
            test_failed++; \
            fprintf(stderr, "    FAIL [%s] %s:%d: %s\n", \
                    current_test, __FILE__, __LINE__, #cond); \
        } \
    } while(0)

#define EXPECT_EQ_INT(a, b) \
    do { \
        long long _a = (long long)(a); long long _b = (long long)(b); \
        if (_a == _b) { test_passed++; } \
        else { \
            test_failed++; \
            fprintf(stderr, "    FAIL [%s] %s:%d: %s (%lld) != %s (%lld)\n", \
                    current_test, __FILE__, __LINE__, #a, _a, #b, _b); \
        } \
    } while(0)

#define EXPECT_NEAR(a, b, eps) \
    do { \
        double _a = (double)(a); double _b = (double)(b); \
        if (fabs(_a - _b) < (eps)) { test_passed++; } \
        else { \
            test_failed++; \
            fprintf(stderr, "    FAIL [%s] %s:%d: %s (%.6f) !~= %s (%.6f)\n", \
                    current_test, __FILE__, __LINE__, #a, _a, #b, _b); \
        } \
    } while(0)

#define TEST_REPORT() \
    do { \
        printf("---\nTotal: %d passed, %d failed\n", test_passed, test_failed); \
        return test_failed == 0 ? 0 : 1; \
    } while(0)

#endif
