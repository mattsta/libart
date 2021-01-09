#pragma once

#include <stdint.h>
__BEGIN_DECLS

typedef enum artIncrementDesc {
    ART_INCREMENT_REPLACE = 0,
    ART_INCREMENT_WHOLE,
    ART_INCREMENT_A,
    ART_INCREMENT_B,
} artIncrementDesc;

typedef union artValue {
    void *ptr;
    uint64_t u;
    int64_t i;
    struct {
        uint32_t a;
        uint32_t b;
    } su;
    struct {
        int32_t a;
        int32_t b;
    } si;
} artValue;

_Static_assert(
    sizeof(artValue) == sizeof(uint64_t),
    "artValue is larger than we expect, are you sure you want it to be "
    "larger than 8 bytes?");

typedef int (*artCallback)(void *data, const void *key, uint32_t keyLen,
                           void *value);

__END_DECLS
