#pragma once

#include "artCommon.h"
__BEGIN_DECLS

typedef enum artType { NODE4 = 1, NODE16, NODE48, NODE256 } artType;

#define MAX_PREFIX_LEN 10

/**
 * This struct is included as part of all the various node sizes
 */
typedef struct artNode {
    uint32_t partialLen; /* length of 'partial' */
#if 0
    uint16_t type:4; /* one of the 'artType' enum values (>= 2 bits) */
    uint16_t childrenCount:12; /* Must be able to hold value 256 (>= 9 bits) */
#else
    /* childrenCount can technically be incremented to 256 which rolls over to
     * zero, but for types NODE48 and NODE256, we don't use childrenCount,
     * we just always iterate through 256 completely.
     * So, even though this is technically wrong, the code has compensated
     * for not using the count at larger node types. */
    uint8_t type;
    uint8_t childrenCount;
#endif
    uint8_t partial[MAX_PREFIX_LEN];
} artNode;

/**
 * Small node with only 4 children
 */
typedef struct artNode4 {
    artNode n;
    uint8_t keys[4];
    artNode *children[4];
} artNode4;

/**
 * Node with 16 children
 */
typedef struct artNode16 {
    artNode n;
    /* 16 keys and children because 16 bytes loads nicely into a __m128i */
    uint8_t keys[16];
    artNode *children[16];
} artNode16;

/**
 * Node with 48 children, but a full 256 byte field.
 */
typedef struct artNode48 {
    artNode n;
    uint8_t keys[256];
    artNode *children[48];
} artNode48;

/**
 * Full node with 256 children
 */
typedef struct artNode256 {
    artNode n;
    /* node256 has no keys and uses pointers directly for comparisons */
    artNode *children[256];
} artNode256;

/**
 * Represents a leaf. These are of arbitrary size, as they include the key.
 */
typedef struct artLeaf {
    void *value;
    uint32_t keyLen;
    uint8_t key[];
} artLeaf;

typedef struct art {
    artNode *root;
    uint64_t count;
} art;

__END_DECLS
