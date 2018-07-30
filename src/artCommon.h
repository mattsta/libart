#pragma once

#include <stdint.h>
__BEGIN_DECLS

typedef int (*artCallback)(void *data, const void *key, uint32_t keyLen,
                           void *value);

__END_DECLS
