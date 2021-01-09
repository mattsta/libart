#pragma once

#include "artCommon.h"
__BEGIN_DECLS

/* 'artType' must fit in 2 bits (max integer value is 3) */
typedef enum artType { NODE4 = 0, NODE16, NODE48, NODE256 } artType;

/**
 * This struct is included as part of all the various node sizes
 */
#if 0
#define MAX_PREFIX_LEN 10
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
#else
/* Optimize MAX_PREFIX_LEN by reducing len and type to minimal required sizes */
#define MAX_PREFIX_LEN 14
typedef struct artNode {
    uint8_t partialLen; /* length of 'partial' (could be 4 bits, but less
                           efficient) */
    uint8_t type : 2;
    uint8_t childrenCount : 6;
    uint8_t partial[MAX_PREFIX_LEN];
} artNode;

_Static_assert(
    sizeof(artNode) == 16,
    "Are you sure you want to make artNode bigger than we expected?");
#endif

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
    /* (16 bytes * 8 bytes/byte == 128 bits == in-place __m128i vector) */
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
struct artLeaf {
    /* TODO: create artSet where we have an art with ONLY keys.
     *       We only need to test for the presence of a key and we can
     *       save 8 bytes since we don't need 'value' in the case of an
     *       artSet */
    /* Also, leaves are allocated individually thus incurring even more
     * allocation overhead. We could create a slab/mempool of leaves to hand out
     * as needed. See function 'make_leaf' */
    artValue value;
    uint32_t keyLen;
    uint8_t key[];
};

typedef struct artKeySetLeaf {
    /* Also, leaves are allocated individually thus incurring even more
     * allocation overhead. We could create a slab/mempool of leaves to hand out
     * as needed. See function 'make_leaf' */
    uint32_t keyLen;
    uint8_t key[];
} artKeySetLeaf;

struct art {
    artNode *root;
    uint64_t count;
};

__END_DECLS
