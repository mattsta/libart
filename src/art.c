#include "art.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#if __SSE__
#include <emmintrin.h>
#endif

/**
 * Macros to manipulate pointer tags
 */
#define IS_LEAF(x) (((uintptr_t)x & 1))
#define SET_LEAF(x) ((void *)((uintptr_t)x | 1))
#define LEAF_RAW(x) ((art_leaf *)((void *)((uintptr_t)x & ~1)))

/**
 * Allocates a node of the given type,
 * initializes to zero and sets the type.
 */
static art_node *alloc_node(uint8_t type) {
    art_node *n;
    switch (type) {
    case NODE4:
        n = (art_node *)calloc(1, sizeof(art_node4));
        break;
    case NODE16:
        n = (art_node *)calloc(1, sizeof(art_node16));
        break;
    case NODE48:
        n = (art_node *)calloc(1, sizeof(art_node48));
        break;
    case NODE256:
        n = (art_node *)calloc(1, sizeof(art_node256));
        break;
    default:
        __builtin_unreachable();
    }

    n->type = type;
    return n;
}

/**
 * Initializes an ART tree
 * @return 0 on success.
 */
int art_tree_init(art_tree *t) {
    t->root = NULL;
    t->size = 0;
    return 0;
}

// Recursively destroys the tree
static void destroy_node(art_node *n) {
    // Break if null
    if (!n) {
        return;
    }

    // Special case leafs
    if (IS_LEAF(n)) {
        free(LEAF_RAW(n));
        return;
    }

    // Handle each node type
    int i, idx;
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    switch (n->type) {
    case NODE4:
        p.p1 = (art_node4 *)n;
        for (i = 0; i < n->childrenCount; i++) {
            destroy_node(p.p1->children[i]);
        }

        break;

    case NODE16:
        p.p2 = (art_node16 *)n;
        for (i = 0; i < n->childrenCount; i++) {
            destroy_node(p.p2->children[i]);
        }

        break;

    case NODE48:
        p.p3 = (art_node48 *)n;
        for (i = 0; i < 256; i++) {
            idx = ((art_node48 *)n)->keys[i];
            if (!idx) {
                continue;
            }

            destroy_node(p.p3->children[idx - 1]);
        }

        break;

    case NODE256:
        p.p4 = (art_node256 *)n;
        for (i = 0; i < 256; i++) {
            if (p.p4->children[i]) {
                destroy_node(p.p4->children[i]);
            }
        }

        break;

    default:
        __builtin_unreachable();
    }

    // Free ourself on the way up
    free(n);
}

/**
 * Destroys an ART tree
 * @return 0 on success.
 */
int art_tree_destroy(art_tree *t) {
    destroy_node(t->root);
    return 0;
}

static size_t countNodes(art_node *n) {
    if (!n) {
        return 0;
    }

    // Special case leafs
    if (IS_LEAF(n)) {
        return 1;
    }

    // Handle each node type
    int i, idx;
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    size_t total = 0;
    switch (n->type) {
    case NODE4:
        p.p1 = (art_node4 *)n;
        for (i = 0; i < n->childrenCount; i++) {
            total += countNodes(p.p1->children[i]);
        }

        break;

    case NODE16:
        p.p2 = (art_node16 *)n;
        for (i = 0; i < n->childrenCount; i++) {
            total += countNodes(p.p2->children[i]);
        }

        break;

    case NODE48:
        p.p3 = (art_node48 *)n;
        for (i = 0; i < 256; i++) {
            idx = ((art_node48 *)n)->keys[i];
            if (!idx) {
                continue;
            }

            total += countNodes(p.p3->children[idx - 1]);
        }

        break;

    case NODE256:
        p.p4 = (art_node256 *)n;
        for (i = 0; i < 256; i++) {
            if (p.p4->children[i]) {
                total += countNodes(p.p4->children[i]);
            }
        }

        break;

    default:
        __builtin_unreachable();
    }

    return total + 1;
}

size_t art_tree_nodes(art_tree *t) {
    return countNodes(t->root);
}

static size_t countBytes(art_node *n) {
    if (!n) {
        return 0;
    }

    // Special case leafs
    if (IS_LEAF(n)) {
        return sizeof(art_leaf) + LEAF_RAW(n)->keyLen;
    }

    // Handle each node type
    int i, idx;
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    size_t total = 0;
    switch (n->type) {
    case NODE4:
        p.p1 = (art_node4 *)n;
        for (i = 0; i < n->childrenCount; i++) {
            total += countBytes(p.p1->children[i]);
        }

        break;

    case NODE16:
        p.p2 = (art_node16 *)n;
        for (i = 0; i < n->childrenCount; i++) {
            total += countBytes(p.p2->children[i]);
        }

        break;

    case NODE48:
        p.p3 = (art_node48 *)n;
        for (i = 0; i < 256; i++) {
            idx = ((art_node48 *)n)->keys[i];
            if (!idx) {
                continue;
            }

            total += countBytes(p.p3->children[idx - 1]);
        }

        break;

    case NODE256:
        p.p4 = (art_node256 *)n;
        for (i = 0; i < 256; i++) {
            if (p.p4->children[i]) {
                total += countBytes(p.p4->children[i]);
            }
        }

        break;

    default:
        __builtin_unreachable();
    }

    return total + sizeof(*n);
}

size_t art_tree_bytes(art_tree *t) {
    return countBytes(t->root);
}

/* =================================================
 * Attempt to fix algorithm deficiency
 * ================================================ */
/* These keyAt() things were originally from:
 https://github.com/wez/watchman/commit/8950a98ba023120621a2665225d754ce28182512
 and
 https://github.com/armon/libart/issues/12 */
// The ART implementation requires that no key be a full prefix of an existing
// key during insertion.  In practice this means that each key must have a
// terminator character.  One approach is to ensure that the key and key_len
// includes a physical trailing NUL terminator when inserting C-strings.
// This doesn't help a great deal when working with binary strings that may be
// a slice in the middle of a buffer that has no termination.
//
// To facilitate this the key_at() function is used to look up the byte
// value at a given index.  If that index is 1 byte after the end of the
// key, we synthesize a fake NUL terminator byte.
//
// Note that if the keys contain NUL bytes earlier in the string this will
// break down and won't have the correct results.
//
// If the index is out of bounds we will assert to trap the fatal coding
// error inside this implementation.
//
// @param key pointer to the key bytes
// @param key_len the size of the byte, in bytes
// @param idx the index into the key
// @return the value of the key at the supplied index.
#if 1
#define keyAt(key, len, idx) ((idx) == (len) ? 0 : (key)[idx])
#else
static inline unsigned char key_at(const unsigned char *key, int key_len,
                                   int idx) {
    if (idx == key_len) {
        // Implicit terminator
        return 0;
    }

    return key[idx];
}
#endif

// A helper for looking at the key value at given index, in a leaf
#if 1
#define leafKeyAt(leaf, idx) keyAt((leaf)->key, (leaf)->keyLen, idx)
#else
static inline unsigned char leaf_key_at(const art_leaf *l, int idx) {
    return keyAt(l->key, l->keyLen, idx);
}
#endif

static art_node **find_child(art_node *n, uint8_t c) {
    union {
        art_node4 *p1;
        art_node16 *p2;
        art_node48 *p3;
        art_node256 *p4;
    } p;
    switch (n->type) {
    case NODE4:
        p.p1 = (art_node4 *)n;
        for (int_fast32_t i = 0; i < n->childrenCount; i++) {
            if ((p.p1->keys)[i] == c) {
                return &p.p1->children[i];
            }
        }

        break;

    case NODE16: {
        p.p2 = (art_node16 *)n;

#if __SSE__
        // Compare the key to all 16 stored keys
        const __m128i cmp = _mm_cmpeq_epi8(
            _mm_set1_epi8(c), _mm_loadu_si128((__m128i *)p.p2->keys));

        // Use a mask to ignore children that don't exist
        const uint_fast32_t mask = (1ULL << n->childrenCount) - 1;
        const uint_fast32_t bitfield = _mm_movemask_epi8(cmp) & mask;
#else
        // Compare the key to all 16 stored keys
        uint_fast32_t bitfield = 0;
        for (int_fast32_t i = 0; i < 16; ++i) {
            if (p.p2->keys[i] == c)
                bitfield |= (1 << i);
        }

        // Use a mask to ignore children that don't exist
        const uint_fast32_t mask = (1 << n->childrenCount) - 1;
        bitfield &= mask;
#endif

        /*
         * If we have a match (any bit set) then we can
         * return the pointer match using ctz to get
         * the index.
         */
        if (bitfield) {
            return &p.p2->children[__builtin_ctz(bitfield)];
        }

        break;
    }

    case NODE48:
        p.p3 = (art_node48 *)n;
        int_fast32_t i = p.p3->keys[c];
        if (i) {
            return &p.p3->children[i - 1];
        }

        break;

    case NODE256:
        p.p4 = (art_node256 *)n;
        if (p.p4->children[c]) {
            return &p.p4->children[c];
        }

        break;

    default:
        __builtin_unreachable();
    }

    return NULL;
}

// Simple inlined if
static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

/**
 * Returns the number of prefix characters shared between
 * the key and node.
 */
static int check_prefix(const art_node *n, const void *key_, int keyLen,
                        int depth) {
    int max_cmp = min(min(n->partialLen, MAX_PREFIX_LEN), keyLen - depth);
    int idx;
    const uint8_t *restrict key = key_;
    for (idx = 0; idx < max_cmp; idx++) {
        if (n->partial[idx] != key[depth + idx]) {
            return idx;
        }
    }

    return idx;
}

/**
 * Checks if a leaf matches
 * @return true on success.
 */
static bool leaf_matches(const art_leaf *n, const void *key, int keyLen) {
    // Fail if key lengths are different
    if (n->keyLen != (uint32_t)keyLen) {
        return false;
    }

    // Compare the keys starting at the depth
    return memcmp(n->key, key, keyLen) == 0;
}

/**
 * Searches for a value in the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg keyLen The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void *art_search(const art_tree *t, const void *key_, int keyLen) {
    art_node **child;
    art_node *n = t->root;
    int prefix_len;
    int depth = 0;

    const uint8_t *restrict key = key_;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            art_leaf *leaf = LEAF_RAW(n);
            // Check if the expanded path matches
            if (leaf_matches(leaf, key, keyLen)) {
                return leaf->value;
            }

            return NULL;
        }

        // Bail if the prefix does not match
        if (n->partialLen) {
            prefix_len = check_prefix(n, key, keyLen, depth);
            if (prefix_len != min(MAX_PREFIX_LEN, n->partialLen)) {
                return NULL;
            }

            depth = depth + n->partialLen;
        }

        if (depth > keyLen) {
            /* Key in tree is longer than input key.
             * Match impossible. */
            return NULL;
        }

        // Recursively search
        child = find_child(n, keyAt(key, keyLen, depth));
        n = (child) ? *child : NULL;
        depth++;
    }

    return NULL;
}

// Find the minimum leaf under a node
static art_leaf *minimum(const art_node *n) {
    // Handle base cases
    if (!n) {
        return NULL;
    }

    if (IS_LEAF(n)) {
        return LEAF_RAW(n);
    }

    int idx;
    switch (n->type) {
    case NODE4:
        return minimum(((const art_node4 *)n)->children[0]);
    case NODE16:
        return minimum(((const art_node16 *)n)->children[0]);
    case NODE48:
        idx = 0;
        while (!((const art_node48 *)n)->keys[idx]) {
            idx++;
        }

        idx = ((const art_node48 *)n)->keys[idx] - 1;
        return minimum(((const art_node48 *)n)->children[idx]);
    case NODE256:
        idx = 0;
        while (!((const art_node256 *)n)->children[idx]) {
            idx++;
        }

        return minimum(((const art_node256 *)n)->children[idx]);
    default:
        __builtin_unreachable();
    }
}

// Find the maximum leaf under a node
static art_leaf *maximum(const art_node *n) {
    // Handle base cases
    if (!n) {
        return NULL;
    }

    if (IS_LEAF(n)) {
        return LEAF_RAW(n);
    }

    int idx;
    switch (n->type) {
    case NODE4:
        return maximum(((const art_node4 *)n)->children[n->childrenCount - 1]);
    case NODE16:
        return maximum(((const art_node16 *)n)->children[n->childrenCount - 1]);
    case NODE48:
        idx = 255;
        while (!((const art_node48 *)n)->keys[idx]) {
            idx--;
        }

        idx = ((const art_node48 *)n)->keys[idx] - 1;
        return maximum(((const art_node48 *)n)->children[idx]);
    case NODE256:
        idx = 255;
        while (!((const art_node256 *)n)->children[idx]) {
            idx--;
        }

        return maximum(((const art_node256 *)n)->children[idx]);
    default:
        __builtin_unreachable();
    }
}

/**
 * Returns the minimum valued leaf
 */
art_leaf *art_minimum(art_tree *t) {
    return minimum((art_node *)t->root);
}

/**
 * Returns the maximum valued leaf
 */
art_leaf *art_maximum(art_tree *t) {
    return maximum((art_node *)t->root);
}

static art_leaf *make_leaf(const void *key, int keyLen, void *value) {
    art_leaf *l = (art_leaf *)calloc(1, sizeof(art_leaf) + keyLen);
    l->value = value;
    l->keyLen = keyLen;
    memcpy(l->key, key, keyLen);
    return l;
}

static int longest_common_prefix(art_leaf *l1, art_leaf *l2, int depth) {
    int max_cmp = min(l1->keyLen, l2->keyLen) - depth;
    int idx;
    for (idx = 0; idx < max_cmp; idx++) {
        if (l1->key[depth + idx] != l2->key[depth + idx]) {
            return idx;
        }
    }

    return idx;
}

static void copy_header(art_node *dest, art_node *src) {
    dest->childrenCount = src->childrenCount;
    dest->partialLen = src->partialLen;
    memcpy(dest->partial, src->partial, min(MAX_PREFIX_LEN, src->partialLen));
}

static void add_child256(art_node256 *n, art_node **ref, uint8_t c,
                         void *child) {
    (void)ref;
    n->n.childrenCount++;
    n->children[c] = (art_node *)child;
}

static void add_child48(art_node48 *n, art_node **ref, uint8_t c, void *child) {
    if (n->n.childrenCount < 48) {
        int pos = 0;
        while (n->children[pos]) {
            pos++;
        }

        n->children[pos] = (art_node *)child;
        n->keys[c] = pos + 1;
        n->n.childrenCount++;
    } else {
        art_node256 *new_node = (art_node256 *)alloc_node(NODE256);
        for (int i = 0; i < 256; i++) {
            if (n->keys[i]) {
                new_node->children[i] = n->children[n->keys[i] - 1];
            }
        }

        copy_header((art_node *)new_node, (art_node *)n);
        *ref = (art_node *)new_node;
        free(n);
        add_child256(new_node, ref, c, child);
    }
}

static void add_child16(art_node16 *n, art_node **ref, uint8_t c, void *child) {
    if (n->n.childrenCount < 16) {
        const uint_fast32_t mask = (1 << n->n.childrenCount) - 1;

#if __SSE__
        // Compare the key to all 16 stored keys
        const __m128i cmp = _mm_cmplt_epi8(_mm_set1_epi8(c),
                                           _mm_loadu_si128((__m128i *)n->keys));

        // Use a mask to ignore children that don't exist
        const uint_fast32_t bitfield = _mm_movemask_epi8(cmp) & mask;
#else
        // Compare the key to all 16 stored keys
        uint_fast32_t bitfield = 0;
        for (short i = 0; i < 16; ++i) {
            if (c < n->keys[i])
                bitfield |= (1 << i);
        }

        // Use a mask to ignore children that don't exist
        bitfield &= mask;
#endif

        // Check if less than any
        uint_fast32_t idx;
        if (bitfield) {
            idx = __builtin_ctz(bitfield);
            memmove(n->keys + idx + 1, n->keys + idx, n->n.childrenCount - idx);
            memmove(n->children + idx + 1, n->children + idx,
                    (n->n.childrenCount - idx) * sizeof(void *));
        } else {
            idx = n->n.childrenCount;
        }

        // Set the child
        n->keys[idx] = c;
        n->children[idx] = (art_node *)child;
        n->n.childrenCount++;
    } else {
        art_node48 *new_node = (art_node48 *)alloc_node(NODE48);

        // Copy the child pointers and populate the key map
        memcpy(new_node->children, n->children,
               sizeof(void *) * n->n.childrenCount);
        for (int_fast32_t i = 0; i < n->n.childrenCount; i++) {
            new_node->keys[n->keys[i]] = i + 1;
        }

        copy_header((art_node *)new_node, (art_node *)n);
        *ref = (art_node *)new_node;
        free(n);
        add_child48(new_node, ref, c, child);
    }
}

static void add_child4(art_node4 *n, art_node **ref, uint8_t c, void *child) {
    if (n->n.childrenCount < 4) {
        int idx;
        for (idx = 0; idx < n->n.childrenCount; idx++) {
            if (c < n->keys[idx]) {
                break;
            }
        }

        // Shift to make room
        memmove(n->keys + idx + 1, n->keys + idx, n->n.childrenCount - idx);
        memmove(n->children + idx + 1, n->children + idx,
                (n->n.childrenCount - idx) * sizeof(void *));

        // Insert element
        n->keys[idx] = c;
        n->children[idx] = (art_node *)child;
        n->n.childrenCount++;
    } else {
        art_node16 *new_node = (art_node16 *)alloc_node(NODE16);

        // Copy the child pointers and the key map
        memcpy(new_node->children, n->children,
               sizeof(void *) * n->n.childrenCount);
        memcpy(new_node->keys, n->keys, sizeof(uint8_t) * n->n.childrenCount);
        copy_header((art_node *)new_node, (art_node *)n);
        *ref = (art_node *)new_node;
        free(n);
        add_child16(new_node, ref, c, child);
    }
}

static void add_child(art_node *n, art_node **ref, uint8_t c, void *child) {
    switch (n->type) {
    case NODE4:
        add_child4((art_node4 *)n, ref, c, child);
        break;
    case NODE16:
        add_child16((art_node16 *)n, ref, c, child);
        break;
    case NODE48:
        add_child48((art_node48 *)n, ref, c, child);
        break;
    case NODE256:
        add_child256((art_node256 *)n, ref, c, child);
        break;
    default:
        __builtin_unreachable();
    }
}

/**
 * Calculates the index at which the prefixes mismatch
 */
static int prefix_mismatch(const art_node *n, const void *key_, int keyLen,
                           int depth) {
    int max_cmp = min(min(MAX_PREFIX_LEN, n->partialLen), keyLen - depth);
    int idx;
    const uint8_t *restrict key = key_;
    for (idx = 0; idx < max_cmp; idx++) {
        if (n->partial[idx] != key[depth + idx]) {
            return idx;
        }
    }

    // If the prefix is short we can avoid finding a leaf
    if (n->partialLen > MAX_PREFIX_LEN) {
        // Prefix is longer than what we've checked, find a leaf
        art_leaf *l = minimum(n);
        max_cmp = min(l->keyLen, keyLen) - depth;
        for (; idx < max_cmp; idx++) {
            if (l->key[idx + depth] != key[depth + idx]) {
                return idx;
            }
        }
    }

    return idx;
}

static void *recursive_insert(art_node *n, art_node **ref, const void *key_,
                              int keyLen, void *value, int depth, int *old) {
    const uint8_t *restrict key = key_;

    // If we are at a NULL node, inject a leaf
    if (!n) {
        *ref = (art_node *)SET_LEAF(make_leaf(key, keyLen, value));
        return NULL;
    }

    // If we are at a leaf, we need to replace it with a node
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);

        // Check if we are updating an existing value
        if (leaf_matches(l, key, keyLen)) {
            *old = 1;
            void *old_val = l->value;
            l->value = value;
            return old_val;
        }

        // New value, we must split the leaf into a node4
        art_node4 *new_node = (art_node4 *)alloc_node(NODE4);

        // Create a new leaf
        art_leaf *l2 = make_leaf(key, keyLen, value);

        // Determine longest prefix
        int longest_prefix = longest_common_prefix(l, l2, depth);
        new_node->n.partialLen = longest_prefix;
        memcpy(new_node->n.partial, key + depth,
               min(MAX_PREFIX_LEN, longest_prefix));
        // Add the leafs to the new node4
        *ref = (art_node *)new_node;
        add_child4(new_node, ref, leafKeyAt(l, depth + longest_prefix),
                   SET_LEAF(l));
        add_child4(new_node, ref, leafKeyAt(l2, depth + longest_prefix),
                   SET_LEAF(l2));
        return NULL;
    }

    // Check if given node has a prefix
    if (n->partialLen) {
        // Determine if the prefixes differ, since we need to split
        int prefix_diff = prefix_mismatch(n, key, keyLen, depth);
        if ((uint32_t)prefix_diff >= n->partialLen) {
            depth += n->partialLen;
            goto RECURSE_SEARCH;
        }

        // Create a new node
        art_node4 *new_node = (art_node4 *)alloc_node(NODE4);
        *ref = (art_node *)new_node;
        new_node->n.partialLen = prefix_diff;
        memcpy(new_node->n.partial, n->partial,
               min(MAX_PREFIX_LEN, prefix_diff));

        // Adjust the prefix of the old node
        if (n->partialLen <= MAX_PREFIX_LEN) {
            add_child4(new_node, ref, n->partial[prefix_diff], n);
            n->partialLen -= (prefix_diff + 1);
            memmove(n->partial, n->partial + prefix_diff + 1,
                    min(MAX_PREFIX_LEN, n->partialLen));
        } else {
            n->partialLen -= (prefix_diff + 1);
            art_leaf *l = minimum(n);
            add_child4(new_node, ref, leafKeyAt(l, depth + prefix_diff), n);
            memcpy(n->partial, l->key + depth + prefix_diff + 1,
                   min(MAX_PREFIX_LEN, n->partialLen));
        }

        // Insert the new leaf
        art_leaf *l = make_leaf(key, keyLen, value);
        add_child4(new_node, ref, leafKeyAt(l, depth + prefix_diff),
                   SET_LEAF(l));
        return NULL;
    }

RECURSE_SEARCH:;
    // Find a child to recurse to
    art_node **child = find_child(n, keyAt(key, keyLen, depth));
    if (child) {
        return recursive_insert(*child, child, key, keyLen, value, depth + 1,
                                old);
    }

    // No child, node goes within us
    art_leaf *l = make_leaf(key, keyLen, value);
    add_child(n, ref, leafKeyAt(l, depth), SET_LEAF(l));
    return NULL;
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
void *art_insert(art_tree *t, const void *key, int keyLen, void *value) {
    int old_val = 0;
    void *old =
        recursive_insert(t->root, &t->root, key, keyLen, value, 0, &old_val);

    if (!old_val) {
        t->size++;
    }

    return old;
}

static void remove_child256(art_node256 *n, art_node **ref, uint8_t c) {
    n->children[c] = NULL;
    n->n.childrenCount--;

    // Resize to a node48 on underflow, not immediately to prevent
    // trashing if we sit on the 48/49 boundary
    if (n->n.childrenCount == 37) {
        art_node48 *new_node = (art_node48 *)alloc_node(NODE48);
        *ref = (art_node *)new_node;
        copy_header((art_node *)new_node, (art_node *)n);

        int pos = 0;
        for (int i = 0; i < 256; i++) {
            if (n->children[i]) {
                new_node->children[pos] = n->children[i];
                new_node->keys[i] = pos + 1;
                pos++;
            }
        }

        free(n);
    }
}

static void remove_child48(art_node48 *n, art_node **ref, uint8_t c) {
    int pos = n->keys[c];
    n->keys[c] = 0;
    n->children[pos - 1] = NULL;
    n->n.childrenCount--;

    if (n->n.childrenCount == 12) {
        art_node16 *new_node = (art_node16 *)alloc_node(NODE16);
        *ref = (art_node *)new_node;
        copy_header((art_node *)new_node, (art_node *)n);

        int child = 0;
        for (int i = 0; i < 256; i++) {
            pos = n->keys[i];
            if (pos) {
                new_node->keys[child] = i;
                new_node->children[child] = n->children[pos - 1];
                child++;
            }
        }

        free(n);
    }
}

static void remove_child16(art_node16 *n, art_node **ref, art_node **l) {
    int pos = l - n->children;
    memmove(n->keys + pos, n->keys + pos + 1, n->n.childrenCount - 1 - pos);
    memmove(n->children + pos, n->children + pos + 1,
            (n->n.childrenCount - 1 - pos) * sizeof(void *));
    n->n.childrenCount--;

    if (n->n.childrenCount == 3) {
        art_node4 *new_node = (art_node4 *)alloc_node(NODE4);
        *ref = (art_node *)new_node;
        copy_header((art_node *)new_node, (art_node *)n);
        memcpy(new_node->keys, n->keys, 4);
        memcpy(new_node->children, n->children, 4 * sizeof(void *));
        free(n);
    }
}

static void remove_child4(art_node4 *n, art_node **ref, art_node **l) {
    int pos = l - n->children;
    memmove(n->keys + pos, n->keys + pos + 1, n->n.childrenCount - 1 - pos);
    memmove(n->children + pos, n->children + pos + 1,
            (n->n.childrenCount - 1 - pos) * sizeof(void *));
    n->n.childrenCount--;

    // Remove nodes with only a single child
    if (n->n.childrenCount == 1) {
        art_node *child = n->children[0];
        if (!IS_LEAF(child)) {
            // Concatenate the prefixes
            int prefix = n->n.partialLen;
            if (prefix < MAX_PREFIX_LEN) {
                n->n.partial[prefix] = n->keys[0];
                prefix++;
            }

            if (prefix < MAX_PREFIX_LEN) {
                int sub_prefix =
                    min(child->partialLen, MAX_PREFIX_LEN - prefix);
                memcpy(n->n.partial + prefix, child->partial, sub_prefix);
                prefix += sub_prefix;
            }

            // Store the prefix in the child
            memcpy(child->partial, n->n.partial, min(prefix, MAX_PREFIX_LEN));
            child->partialLen += n->n.partialLen + 1;
        }

        *ref = child;
        free(n);
    }
}

static void remove_child(art_node *n, art_node **ref, uint8_t c, art_node **l) {
    switch (n->type) {
    case NODE4:
        remove_child4((art_node4 *)n, ref, l);
        break;
    case NODE16:
        remove_child16((art_node16 *)n, ref, l);
        break;
    case NODE48:
        remove_child48((art_node48 *)n, ref, c);
        break;
    case NODE256:
        remove_child256((art_node256 *)n, ref, c);
        break;
    default:
        __builtin_unreachable();
    }
}

static art_leaf *recursive_delete(art_node *n, art_node **ref, const void *key_,
                                  int keyLen, int depth) {
    // Search terminated
    if (!n) {
        return NULL;
    }

    const uint8_t *restrict key = key_;

    // Handle hitting a leaf node
    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);
        if (leaf_matches(l, key, keyLen)) {
            *ref = NULL;
            return l;
        }

        return NULL;
    }

    // Bail if the prefix does not match
    if (n->partialLen) {
        int prefix_len = check_prefix(n, key, keyLen, depth);
        if (prefix_len != min(MAX_PREFIX_LEN, n->partialLen)) {
            return NULL;
        }

        depth = depth + n->partialLen;
    }

    // Find child node
    art_node **child = find_child(n, keyAt(key, keyLen, depth));
    if (!child) {
        return NULL;
    }

    // If the child is leaf, delete from this node
    if (IS_LEAF(*child)) {
        art_leaf *l = LEAF_RAW(*child);
        if (leaf_matches(l, key, keyLen)) {
            remove_child(n, ref, keyAt(key, keyLen, depth), child);
            return l;
        }

        return NULL;

        // Recurse
    }

    return recursive_delete(*child, child, key, keyLen, depth + 1);
}

/**
 * Deletes a value from the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg keyLen The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void *art_delete(art_tree *t, const void *key, int keyLen) {
    art_leaf *l = recursive_delete(t->root, &t->root, key, keyLen, 0);
    if (l) {
        t->size--;
        void *old = l->value;
        free(l);
        return old;
    }

    return NULL;
}

// Recursively iterates over the tree
static int recursive_iter(art_node *n, art_callback cb, void *data) {
    // Handle base cases
    if (!n) {
        return 0;
    }

    if (IS_LEAF(n)) {
        art_leaf *l = LEAF_RAW(n);
        return cb(data, l->key, l->keyLen, l->value);
    }

    int idx;
    int res;
    switch (n->type) {
    case NODE4:
        for (int i = 0; i < n->childrenCount; i++) {
            res = recursive_iter(((art_node4 *)n)->children[i], cb, data);
            if (res) {
                return res;
            }
        }

        break;

    case NODE16:
        for (int i = 0; i < n->childrenCount; i++) {
            res = recursive_iter(((art_node16 *)n)->children[i], cb, data);
            if (res) {
                return res;
            }
        }

        break;

    case NODE48:
        for (int i = 0; i < 256; i++) {
            idx = ((art_node48 *)n)->keys[i];
            if (!idx) {
                continue;
            }

            res =
                recursive_iter(((art_node48 *)n)->children[idx - 1], cb, data);
            if (res) {
                return res;
            }
        }

        break;

    case NODE256:
        for (int i = 0; i < 256; i++) {
            if (!((art_node256 *)n)->children[i]) {
                continue;
            }

            res = recursive_iter(((art_node256 *)n)->children[i], cb, data);
            if (res) {
                return res;
            }
        }

        break;

    default:
        __builtin_unreachable();
    }

    return 0;
}

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
int art_iter(art_tree *t, art_callback cb, void *data) {
    return recursive_iter(t->root, cb, data);
}

/**
 * Checks if a leaf prefix matches
 * @return true on success.
 */
static bool leaf_prefix_matches(const art_leaf *n, const void *prefix,
                                int prefix_len) {
    // Fail if the key length is too short
    if (n->keyLen < (uint32_t)prefix_len) {
        return false;
    }

    // Compare the keys
    return memcmp(n->key, prefix, prefix_len) == 0;
}

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
int art_iter_prefix(art_tree *t, const void *key_, int keyLen, art_callback cb,
                    void *data) {
    art_node **child;
    art_node *n = t->root;
    int prefix_len;
    int depth = 0;
    const uint8_t *restrict key = key_;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = (art_node *)LEAF_RAW(n);
            // Check if the expanded path matches
            if (leaf_prefix_matches((art_leaf *)n, key, keyLen)) {
                art_leaf *l = (art_leaf *)n;
                return cb(data, l->key, l->keyLen, l->value);
            }

            return 0;
        }

        // If the depth matches the prefix, we need to handle this node
        if (depth == keyLen) {
            art_leaf *l = minimum(n);
            if (leaf_prefix_matches(l, key, keyLen)) {
                return recursive_iter(n, cb, data);
            }

            return 0;
        }

        // Bail if the prefix does not match
        if (n->partialLen) {
            prefix_len = prefix_mismatch(n, key, keyLen, depth);

            // Guard if the mis-match is longer than the MAX_PREFIX_LEN
            if ((uint32_t)prefix_len > n->partialLen) {
                prefix_len = n->partialLen;
            }

            // If there is no match, search is terminated
            if (!prefix_len) {
                return 0;

                // If we've matched the prefix, iterate on this node
            } else if (depth + prefix_len == keyLen) {
                return recursive_iter(n, cb, data);
            }

            // if there is a full match, go deeper
            depth = depth + n->partialLen;
        }

        // Recursively search
        child = find_child(n, keyAt(key, keyLen, depth));
        n = (child) ? *child : NULL;
        depth++;
    }

    return 0;
}
