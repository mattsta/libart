#pragma once

#include <stddef.h> /* size_t */
#include <stdint.h>

__BEGIN_DECLS

typedef enum artType { NODE4 = 1, NODE16, NODE48, NODE256 } artType;

#define MAX_PREFIX_LEN 10

typedef int (*art_callback)(void *data, const void *key, uint32_t keyLen,
                            void *value);

/**
 * This struct is included as part of all the various node sizes
 */
typedef struct art_node {
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
} art_node;

/**
 * Small node with only 4 children
 */
typedef struct art_node4 {
    art_node n;
    uint8_t keys[4];
    art_node *children[4];
} art_node4;

/**
 * Node with 16 children
 */
typedef struct art_node16 {
    art_node n;
    /* 16 keys and children because 16 bytes loads nicely into a __m128i */
    uint8_t keys[16];
    art_node *children[16];
} art_node16;

/**
 * Node with 48 children, but a full 256 byte field.
 */
typedef struct art_node48 {
    art_node n;
    uint8_t keys[256];
    art_node *children[48];
} art_node48;

/**
 * Full node with 256 children
 */
typedef struct art_node256 {
    art_node n;
    /* node256 has no keys and uses pointers directly for comparisons */
    art_node *children[256];
} art_node256;

/**
 * Represents a leaf. These are of arbitrary size, as they include the key.
 */
typedef struct art_leaf {
    void *value;
    uint32_t keyLen;
    uint8_t key[];
} art_leaf;

/**
 * Main struct, points to root.
 */
typedef struct art_tree {
    art_node *root;
    uint64_t size;
} art_tree;

/**
 * Initializes an ART tree
 * @return 0 on success.
 */
int art_tree_init(art_tree *t);

/**
 * DEPRECATED
 * Initializes an ART tree
 * @return 0 on success.
 */
#define init_art_tree(...) art_tree_init(__VA_ARGS__)

/**
 * Destroys an ART tree
 * @return 0 on success.
 */
int art_tree_destroy(art_tree *t);
size_t art_tree_bytes(art_tree *t);
size_t art_tree_nodes(art_tree *t);

/**
 * DEPRECATED
 * Initializes an ART tree
 * @return 0 on success.
 */
#define destroy_art_tree(...) art_tree_destroy(__VA_ARGS__)

/**
 * Returns the size of the ART tree.
 */
inline uint64_t art_size(art_tree *t) {
    return t->size;
}

/**
 * Inserts a new value into the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg keyLen The length of the key
 * @arg value Opaque value.
 * @return NULL if the item was newly inserted, otherwise
 * the old value pointer is returned.
 */
void *art_insert(art_tree *t, const void *key, int keyLen, void *value);

/**
 * Deletes a value from the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg keyLen The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void *art_delete(art_tree *t, const void *key, int keyLen);

/**
 * Searches for a value in the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg keyLen The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void *art_search(const art_tree *t, const void *key, int keyLen);

/**
 * Returns the minimum valued leaf
 * @return The minimum leaf or NULL
 */
art_leaf *art_minimum(art_tree *t);

/**
 * Returns the maximum valued leaf
 * @return The maximum leaf or NULL
 */
art_leaf *art_maximum(art_tree *t);

/**
 * Iterates through the entries pairs in the map,
 * invoking a callback for each. The call back gets a
 * key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @arg t The tree to iterate over
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int art_iter(art_tree *t, art_callback cb, void *data);

/**
 * Iterates through the entries pairs in the map,
 * invoking a callback for each that matches a given prefix.
 * The call back gets a key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @arg t The tree to iterate over
 * @arg prefix The prefix of keys to read
 * @arg prefix_len The length of the prefix
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int art_iter_prefix(art_tree *t, const void *prefix, int prefix_len,
                    art_callback cb, void *data);

__END_DECLS
