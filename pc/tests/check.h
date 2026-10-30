#ifndef POKEBW2_PC_TEST_CHECK_H
#define POKEBW2_PC_TEST_CHECK_H
#include <stdio.h>
#include <stdlib.h>

// Test operations and checks must run in Release builds too.
#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                              \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)

#endif
