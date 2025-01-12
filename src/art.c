#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "art.h"
#include "artInternal.h"

#if __SSE__
#include <emmintrin.h>
#endif

#ifndef ART_USE_IMPLICIT_KEY_NULL_TERMINIATOR_PROTECTION
#define ART_USE_IMPLICIT_KEY_NULL_TERMINIATOR_PROTECTION 1
#endif

/**
 * Macros to manipulate pointer tags
 */
#define IS_LEAF(x) (((uintptr_t)x & 1))
#define SET_LEAF(x) ((void *)((uintptr_t)x | 1))
#define LEAF_RAW(x) ((artLeaf *)((void *)((uintptr_t)x & ~1ULL)))

/**
 * Allocates a node of the given type,
 * initializes to zero and sets the type.
 */
static artNode *alloc_node(artType type) {
    artNode *n;
    switch (type) {
    case NODE4:
        n = (artNode *)calloc(1, sizeof(artNode4));
        break;
    case NODE16:
        n = (artNode *)calloc(1, sizeof(artNode16));
        break;
    case NODE48:
        n = (artNode *)calloc(1, sizeof(artNode48));
        break;
    case NODE256:
        n = (artNode *)calloc(1, sizeof(artNode256));
        break;
    default:
        assert(NULL && "Bad type?");
        __builtin_unreachable();
    }

    n->type = type;
    return n;
}

/**
 * Initializes an ART tree
 */
void artInit(art *t) {
    t->root = NULL;
    t->count = 0;
}

art *artNew(void) {
    /* No init needed because init just sets everything to zero anyway. */
    art *t = calloc(1, sizeof(*t));
    return t;
}

// Recursively destroys the tree
static void destroy_node(artNode *n) {
    if (!n) {
        return;
    }

    // Special case leafs
    if (IS_LEAF(n)) {
        free(LEAF_RAW(n));
        return;
    }

    // Handle each node type
    union {
        artNode4 *p1;
        artNode16 *p2;
        artNode48 *p3;
        artNode256 *p4;
        void *any;
    } p = {.any = n};

    switch (n->type) {
    case NODE4:
        for (size_t i = 0; i < n->childrenCount; i++) {
            destroy_node(p.p1->children[i]);
        }

        break;
    case NODE16:
        for (size_t i = 0; i < n->childrenCount; i++) {
            destroy_node(p.p2->children[i]);
        }

        break;
    case NODE48:
        for (size_t i = 0; i < 256; i++) {
            size_t idx = ((artNode48 *)n)->keys[i];
            if (!idx) {
                continue;
            }
            destroy_node(p.p3->children[idx - 1]);
        }

        break;
    case NODE256:
        for (size_t i = 0; i < 256; i++) {
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
 */
void artFreeInner(art *t) {
    destroy_node(t->root);
}

void artFree(art *t) {
    artFreeInner(t);
    free(t);
}

static size_t countNodes(const artNode *n) {
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
        const artNode4 *p1;
        const artNode16 *p2;
        const artNode48 *p3;
        const artNode256 *p4;
        const void *any;
    } p = {.any = n};
    size_t total = 0;

    switch (n->type) {
    case NODE4:
        for (i = 0; i < n->childrenCount; i++) {
            total += countNodes(p.p1->children[i]);
        }

        break;

    case NODE16:
        for (i = 0; i < n->childrenCount; i++) {
            total += countNodes(p.p2->children[i]);
        }

        break;

    case NODE48:
        for (i = 0; i < 256; i++) {
            idx = ((artNode48 *)n)->keys[i];
            if (!idx) {
                continue;
            }
            total += countNodes(p.p3->children[idx - 1]);
        }

        break;

    case NODE256:
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

size_t artNodes(const art *t) {
    return countNodes(t->root);
}

static size_t countBytes(const artNode *n) {
    if (!n) {
        return 0;
    }

    // Special case leafs
    if (IS_LEAF(n)) {
        return sizeof(artLeaf) + LEAF_RAW(n)->keyLen;
    }

    // Handle each node type
    int i, idx;
    union {
        const artNode4 *p1;
        const artNode16 *p2;
        const artNode48 *p3;
        const artNode256 *p4;
        const void *any;
    } p = {.any = n};
    size_t total = 0;

    switch (n->type) {
    case NODE4:
        for (i = 0; i < n->childrenCount; i++) {
            total += countBytes(p.p1->children[i]);
        }

        break;

    case NODE16:
        for (i = 0; i < n->childrenCount; i++) {
            total += countBytes(p.p2->children[i]);
        }

        break;

    case NODE48:
        for (i = 0; i < 256; i++) {
            idx = ((artNode48 *)n)->keys[i];
            if (!idx) {
                continue;
            }
            total += countBytes(p.p3->children[idx - 1]);
        }

        break;

    case NODE256:
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

size_t artBytes(const art *t) {
    return countBytes(t->root);
}

uint64_t artCount(const art *t) {
    return t->count;
}

/* =================================================
 * Attempt to fix algorithm deficiency
 * ================================================ */
/* These keyAt() things were originally from:
 https://github.com/wez/watchman/commit/8950a98ba023120621a2665225d754ce28182512
 and
 https://github.com/armon/libart/issues/12
 also see
 https://github.com/kellydunn/go-art/issues/6 */
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
// break down and won't give correct iteration search results.
//
// @param key pointer to the key bytes
// @param key_len the size of the byte, in bytes
// @param idx the index into the key
// @return the value of the key at the supplied index.
#if ART_USE_IMPLICIT_KEY_NULL_TERMINIATOR_PROTECTION
#define keyAt(key, len, idx) ((idx) == (len) ? '\0' : (key)[idx])
#else /* You KNOW none of your keys are full prefixes of each other. */
#define keyAt(key, len, idx) (key)[idx]
#endif

// A helper for looking at the key value at given index, in a leaf
#define leafKeyAt(leaf, idx) keyAt((leaf)->key, (leaf)->keyLen, idx)

static artNode **find_child(artNode *n, uint8_t c) {
    union {
        artNode4 *p1;
        artNode16 *p2;
        artNode48 *p3;
        artNode256 *p4;
        void *any;
    } p = {.any = n};

    switch (n->type) {
    case NODE4:
        for (int_fast32_t i = 0; i < n->childrenCount; i++) {
            if ((p.p1->keys)[i] == c) {
                return &p.p1->children[i];
            }
        }

        break;

    case NODE16: {
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

    case NODE48: {
        const int_fast32_t i = p.p3->keys[c];
        if (i) {
            return &p.p3->children[i - 1];
        }

        break;
    }

    case NODE256:
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
static int checkPrefix(const artNode *n, const void *key_,
                       const uint_fast32_t keyLen, int depth) {
    int idx;
    const int max_cmp = min(min(n->partialLen, MAX_PREFIX_LEN), keyLen - depth);
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
static inline bool leafNodeIsExactKey(const artLeaf *restrict const n,
                                      const void *restrict const key,
                                      const uint_fast32_t keyLen) {
    if (n->keyLen != keyLen) {
        return false;
    }

    return memcmp(n->key, key, keyLen) == 0;
}

/**
 * Searches for a value in the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg keyLen The length of the key
 * @arg value value of key
 * @return 'true' if item found, 'false' if not found.
 *
 */
bool artSearch(const art *t, const void *key_, const uint_fast32_t keyLen,
               void **value) {
    artNode **child;
    artNode *n = t->root;
    int prefixLen;
    int depth = 0;

    const uint8_t *restrict key = key_;
    while (n) {
        if (IS_LEAF(n)) {
            artLeaf *leaf = LEAF_RAW(n);

            // Check if the expanded path matches
            if (leafNodeIsExactKey(leaf, key, keyLen)) {
                if (value) {
                    *value = leaf->value.ptr;
                }

                return true;
            }

            return false;
        }

        // Bail if the prefix does not match
        if (n->partialLen) {
            prefixLen = checkPrefix(n, key, keyLen, depth);
            if (prefixLen != min(MAX_PREFIX_LEN, n->partialLen)) {
                return NULL;
            }

            depth = depth + n->partialLen;
        }

        if (depth > keyLen) {
            /* Key in tree is longer than input key.
             * Match impossible. */
            return NULL;
        }

        // Don't overflow the key buffer if we go too deep
        if (depth >= keyLen) {
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
static artLeaf *minimum(const artNode *n) {
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
        return minimum(((const artNode4 *)n)->children[0]);
    case NODE16:
        return minimum(((const artNode16 *)n)->children[0]);
    case NODE48:
        idx = 0;
        while (!((const artNode48 *)n)->keys[idx]) {
            idx++;
        }

        idx = ((const artNode48 *)n)->keys[idx] - 1;
        return minimum(((const artNode48 *)n)->children[idx]);
    case NODE256:
        idx = 0;
        while (!((const artNode256 *)n)->children[idx]) {
            idx++;
        }

        return minimum(((const artNode256 *)n)->children[idx]);
    default:
        __builtin_unreachable();
    }
}

// Find the maximum leaf under a node
static artLeaf *maximum(const artNode *n) {
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
        return maximum(((artNode4 *)n)->children[n->childrenCount - 1]);
    case NODE16:
        return maximum(((artNode16 *)n)->children[n->childrenCount - 1]);
    case NODE48:
        idx = 255;
        while (!((const artNode48 *)n)->keys[idx]) {
            idx--;
        }

        idx = ((const artNode48 *)n)->keys[idx] - 1;
        return maximum(((const artNode48 *)n)->children[idx]);
    case NODE256:
        idx = 255;
        while (!((const artNode256 *)n)->children[idx]) {
            idx--;
        }

        return maximum(((const artNode256 *)n)->children[idx]);
    default:
        __builtin_unreachable();
    }
}

/**
 * Returns the minimum valued leaf
 */
artLeaf *artMinimum(art *t) {
    return minimum((artNode *)t->root);
}

/**
 * Returns the maximum valued leaf
 */
artLeaf *artMaximum(art *t) {
    return maximum((artNode *)t->root);
}

void *artLeafValue(artLeaf *l) {
    return l->value.ptr;
}

size_t artLeafKey(artLeaf *l, void **key) {
    *key = l->key;
    return l->keyLen;
}

void *artLeafKeyOnly(artLeaf *l) {
    return l->key;
}

static artLeaf *make_leaf(const void *key, const uint_fast32_t keyLen,
                          const artValue *value) {
    artLeaf *l = (artLeaf *)calloc(1, sizeof(artLeaf) + keyLen);
    l->value = *value;
    l->keyLen = keyLen;
    memcpy(l->key, key, keyLen);
    return l;
}

static int longest_commonPrefix(artLeaf *l1, artLeaf *l2, int depth) {
    int max_cmp = min(l1->keyLen, l2->keyLen) - depth;
    int idx;
    for (idx = 0; idx < max_cmp; idx++) {
        if (l1->key[depth + idx] != l2->key[depth + idx]) {
            return idx;
        }
    }

    return idx;
}

static void copy_header(artNode *restrict dest, artNode *restrict src) {
    dest->childrenCount = src->childrenCount;
    dest->partialLen = src->partialLen;
    memcpy(dest->partial, src->partial, min(MAX_PREFIX_LEN, src->partialLen));
}

static void add_child256(artNode256 *n, artNode **ref, uint8_t c, void *child) {
    (void)ref;
    n->n.childrenCount++;
    n->children[c] = (artNode *)child;
}

static void add_child48(artNode48 *n, artNode **ref, uint8_t c, void *child) {
    if (n->n.childrenCount < 48) {
        int pos = 0;
        while (n->children[pos]) {
            pos++;
        }

        n->children[pos] = (artNode *)child;
        n->keys[c] = pos + 1;
        n->n.childrenCount++;
    } else {
        artNode256 *new_node = (artNode256 *)alloc_node(NODE256);
        for (int i = 0; i < 256; i++) {
            if (n->keys[i]) {
                new_node->children[i] = n->children[n->keys[i] - 1];
            }
        }

        copy_header((artNode *)new_node, (artNode *)n);
        *ref = (artNode *)new_node;

        free(n);

        add_child256(new_node, ref, c, child);
    }
}

static void add_child16(artNode16 *n, artNode **ref, uint8_t c, void *child) {
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
        n->children[idx] = (artNode *)child;
        n->n.childrenCount++;
    } else {
        artNode48 *new_node = (artNode48 *)alloc_node(NODE48);

        // Copy the child pointers and populate the key map
        memcpy(new_node->children, n->children,
               sizeof(void *) * n->n.childrenCount);
        for (int_fast32_t i = 0; i < n->n.childrenCount; i++) {
            new_node->keys[n->keys[i]] = i + 1;
        }

        copy_header((artNode *)new_node, (artNode *)n);
        *ref = (artNode *)new_node;
        free(n);
        add_child48(new_node, ref, c, child);
    }
}

static void add_child4(artNode4 *n, artNode **ref, uint8_t c, void *child) {
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
        n->children[idx] = (artNode *)child;
        n->n.childrenCount++;
    } else {
        artNode16 *new_node = (artNode16 *)alloc_node(NODE16);

        // Copy the child pointers and the key map
        memcpy(new_node->children, n->children,
               sizeof(void *) * n->n.childrenCount);
        memcpy(new_node->keys, n->keys, sizeof(uint8_t) * n->n.childrenCount);
        copy_header((artNode *)new_node, (artNode *)n);
        *ref = (artNode *)new_node;
        free(n);
        add_child16(new_node, ref, c, child);
    }
}

static void add_child(artNode *n, artNode **ref, uint8_t c, void *child) {
    switch (n->type) {
    case NODE4:
        add_child4((artNode4 *)n, ref, c, child);
        break;
    case NODE16:
        add_child16((artNode16 *)n, ref, c, child);
        break;
    case NODE48:
        add_child48((artNode48 *)n, ref, c, child);
        break;
    case NODE256:
        add_child256((artNode256 *)n, ref, c, child);
        break;
    default:
        __builtin_unreachable();
    }
}

/**
 * Calculates the index at which the prefixes mismatch
 */
static size_t prefix_mismatch(const artNode *n, const void *key_,
                              const uint_fast32_t keyLen, int depth) {
    int max_cmp = min(min(MAX_PREFIX_LEN, n->partialLen), keyLen - depth);
    size_t idx;
    const uint8_t *restrict key = key_;
    for (idx = 0; idx < max_cmp; idx++) {
        if (n->partial[idx] != key[depth + idx]) {
            return idx;
        }
    }

    // If the prefix is short we can avoid finding a leaf
    if (n->partialLen > MAX_PREFIX_LEN) {
        // Prefix is longer than what we've checked, find a leaf
        artLeaf *l = minimum(n);
        max_cmp = min(l->keyLen, keyLen) - depth;
        for (; idx < max_cmp; idx++) {
            if (l->key[idx + depth] != key[depth + idx]) {
                return idx;
            }
        }
    }

    return idx;
}

static void *recursive_insert(artNode *restrict const n, artNode **ref,
                              const void *key_, const uint_fast32_t keyLen,
                              const artValue *const value, int depth,
                              bool *restrict const replaced,
                              const artIncrementDesc desc, artLeaf **usedLeaf) {
    const uint8_t *restrict key = key_;

    // If we are at a NULL node, inject a leaf
    if (!n) {
        artLeaf *restrict const l = make_leaf(key, keyLen, value);
        if (usedLeaf) {
            *usedLeaf = l;
        }

        *ref = (artNode *)SET_LEAF(l);
        return NULL;
    }

    // If we are at a leaf, we need to replace it with a node
    if (IS_LEAF(n)) {
        artLeaf *l = LEAF_RAW(n);
        if (usedLeaf) {
            *usedLeaf = l;
        }

        // Check if we are updating an existing value
        if (leafNodeIsExactKey(l, key, keyLen)) {
            *replaced = true;
            void *const old_val = l->value.ptr;

            switch (desc) {
            case ART_INCREMENT_WHOLE:
                l->value.u++;
                break;
            case ART_INCREMENT_A:
                l->value.su.a++;
                break;
            case ART_INCREMENT_B:
                l->value.su.b++;
                break;
            default:
                l->value = *value;
            }

            return old_val;
        }

        // New value, we must split the leaf into a node4
        artNode4 *new_node = (artNode4 *)alloc_node(NODE4);

        // Create a new leaf
        artLeaf *l2 = make_leaf(key, keyLen, value);

        // Determine longest prefix
        int longestPrefix = longest_commonPrefix(l, l2, depth);
        new_node->n.partialLen = longestPrefix;
        memcpy(new_node->n.partial, key + depth,
               min(MAX_PREFIX_LEN, longestPrefix));

        // Add the leafs to the new node4
        *ref = (artNode *)new_node;
        add_child4(new_node, ref, leafKeyAt(l, depth + longestPrefix),
                   SET_LEAF(l));
        add_child4(new_node, ref, leafKeyAt(l2, depth + longestPrefix),
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
        artNode4 *new_node = (artNode4 *)alloc_node(NODE4);
        *ref = (artNode *)new_node;
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
            artLeaf *l = minimum(n);
            add_child4(new_node, ref, leafKeyAt(l, depth + prefix_diff), n);
            memcpy(n->partial, l->key + depth + prefix_diff + 1,
                   min(MAX_PREFIX_LEN, n->partialLen));
        }

        // Insert the new leaf
        artLeaf *l = make_leaf(key, keyLen, value);
        if (usedLeaf) {
            *usedLeaf = l;
        }

        add_child4(new_node, ref, leafKeyAt(l, depth + prefix_diff),
                   SET_LEAF(l));
        return NULL;
    }

RECURSE_SEARCH:;
    // Find a child to recurse to
    artNode **child = find_child(n, keyAt(key, keyLen, depth));
    if (child) {
        return recursive_insert(*child, child, key, keyLen, value, depth + 1,
                                replaced, desc, usedLeaf);
    }

    // No child, node goes within us
    artLeaf *l = make_leaf(key, keyLen, value);
    if (usedLeaf) {
        *usedLeaf = l;
    }

    add_child(n, ref, leafKeyAt(l, depth), SET_LEAF(l));
    return NULL;
}

/**
 * Inserts a new value into the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg keyLen The length of the key
 * @arg value Opaque value.
 * @return 'true' if key is new; 'false' if key is replaced.
 */
bool artInsert(art *restrict const t, const void *restrict const key,
               const uint_fast32_t keyLen, void *restrict const value_,
               void **oldValue) {
    bool replaced = false;
    const artValue value = {.ptr = value_};
    void *const old =
        recursive_insert(t->root, &t->root, key, keyLen, &value, 0, &replaced,
                         ART_INCREMENT_REPLACE, NULL);

    if (!replaced) {
        t->count++;
        return true;
    }

    if (oldValue) {
        *oldValue = old;
    }

    return false;
}

void artLeafIncrement(artLeaf *l) {
    l->value.u++;
}

bool artInsertIncrement(art *const t, const void *const key,
                        const uint_fast32_t keyLen, const artIncrementDesc desc,
                        artLeaf **usedLeaf) {
    bool replaced = false;

    /* Generate a mock pointer union for _initial_ increment value (if this is
     * the first insert for 'key')... */
    artValue initialValU = {0};
    switch (desc) {
    case ART_INCREMENT_WHOLE:
        initialValU.u = 1;
        break;
    case ART_INCREMENT_A:
        initialValU.su.a = 1;
        break;
    case ART_INCREMENT_B:
        initialValU.su.b = 1;
        break;
    default:
        assert(NULL && "Increment without increment operation?");
        __builtin_unreachable();
    }

    recursive_insert(t->root, &t->root, key, keyLen, &initialValU, 0, &replaced,
                     desc, usedLeaf);

    if (!replaced) {
        t->count++;

        /* Return 'false' meaning key was _inserted_ with start value 1. */
        return false;
    }

    /* Return 'true' meaning key was _incremented_ to a higher value. */
    return true;
}

static void remove_child256(artNode256 *n, artNode **ref, uint8_t c) {
    n->children[c] = NULL;
    n->n.childrenCount--;

    // Resize to a node48 on underflow, not immediately to prevent
    // trashing if we sit on the 48/49 boundary
    if (n->n.childrenCount == 37) {
        artNode48 *new_node = (artNode48 *)alloc_node(NODE48);
        *ref = (artNode *)new_node;
        copy_header((artNode *)new_node, (artNode *)n);

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

static void remove_child48(artNode48 *n, artNode **ref, uint8_t c) {
    int pos = n->keys[c];
    n->keys[c] = 0;
    n->children[pos - 1] = NULL;
    n->n.childrenCount--;

    if (n->n.childrenCount == 12) {
        artNode16 *new_node = (artNode16 *)alloc_node(NODE16);
        *ref = (artNode *)new_node;
        copy_header((artNode *)new_node, (artNode *)n);

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

static void remove_child16(artNode16 *n, artNode **ref, artNode **l) {
    int pos = l - n->children;
    memmove(n->keys + pos, n->keys + pos + 1, n->n.childrenCount - 1 - pos);
    memmove(n->children + pos, n->children + pos + 1,
            (n->n.childrenCount - 1 - pos) * sizeof(void *));
    n->n.childrenCount--;

    if (n->n.childrenCount == 3) {
        artNode4 *new_node = (artNode4 *)alloc_node(NODE4);
        *ref = (artNode *)new_node;
        copy_header((artNode *)new_node, (artNode *)n);
        memcpy(new_node->keys, n->keys, 4);
        memcpy(new_node->children, n->children, 4 * sizeof(void *));
        free(n);
    }
}

static void remove_child4(artNode4 *n, artNode **ref, artNode **l) {
    int pos = l - n->children;
    memmove(n->keys + pos, n->keys + pos + 1, n->n.childrenCount - 1 - pos);
    memmove(n->children + pos, n->children + pos + 1,
            (n->n.childrenCount - 1 - pos) * sizeof(void *));
    n->n.childrenCount--;

    // Remove nodes with only a single child
    if (n->n.childrenCount == 1) {
        artNode *child = n->children[0];
        if (!IS_LEAF(child)) {
            // Concatenate the prefixes
            int prefix = n->n.partialLen;
            if (prefix < MAX_PREFIX_LEN) {
                n->n.partial[prefix] = n->keys[0];
                prefix++;
            }

            if (prefix < MAX_PREFIX_LEN) {
                const int subPrefix =
                    min(child->partialLen, MAX_PREFIX_LEN - prefix);
                memcpy(n->n.partial + prefix, child->partial, subPrefix);
                prefix += subPrefix;
            }

            // Store the prefix in the child
            memcpy(child->partial, n->n.partial, min(prefix, MAX_PREFIX_LEN));
            child->partialLen += n->n.partialLen + 1;
        }

        *ref = child;
        free(n);
    }
}

static void remove_child(artNode *n, artNode **ref, uint8_t c, artNode **l) {
    switch (n->type) {
    case NODE4:
        remove_child4((artNode4 *)n, ref, l);
        break;
    case NODE16:
        remove_child16((artNode16 *)n, ref, l);
        break;
    case NODE48:
        remove_child48((artNode48 *)n, ref, c);
        break;
    case NODE256:
        remove_child256((artNode256 *)n, ref, c);
        break;
    default:
        __builtin_unreachable();
    }
}

static artLeaf *recursive_delete(artNode *restrict const n, artNode **ref,
                                 const void *key_, const uint_fast32_t keyLen,
                                 int depth, const artIncrementDesc desc) {
    // Search terminated
    if (!n) {
        return NULL;
    }

    const uint8_t *restrict key = key_;

    // Handle hitting a leaf node
    if (IS_LEAF(n)) {
        artLeaf *l = LEAF_RAW(n);
        if (leafNodeIsExactKey(l, key, keyLen)) {
            switch (desc) {
            case ART_INCREMENT_WHOLE:
                /* If we were the last refcount, the key is now deleted */
                if (l->value.u == 1) {
                    *ref = NULL;
                } else {
                    /* else, we have more refcounts to delete later */
                    l->value.u--;
                    return NULL;
                }
                break;
            case ART_INCREMENT_A:
                /* If we were the last refcount, the key is now deleted */
                if (l->value.su.a == 1) {
                    *ref = NULL;
                } else {
                    /* else, we have more refcounts to delete later */
                    l->value.su.a--;
                    return NULL;
                }
                break;
            case ART_INCREMENT_B:
                /* If we were the last refcount, the key is now deleted */
                if (l->value.su.b == 1) {
                    *ref = NULL;
                } else {
                    /* else, we have more refcounts to delete later */
                    l->value.su.b--;
                    return NULL;
                }
                break;
            case ART_INCREMENT_REPLACE:
                /* Default "delete means delete" action. */
                *ref = NULL;
                break;
            default:
                assert(NULL && "Unknown action?");
                __builtin_unreachable();
            }

            return l;
        }

        return NULL;
    }

    // Bail if the prefix does not match
    if (n->partialLen) {
        int prefixLen = checkPrefix(n, key, keyLen, depth);
        if (prefixLen != min(MAX_PREFIX_LEN, n->partialLen)) {
            return NULL;
        }

        depth = depth + n->partialLen;
    }

    // Find child node
    artNode **child = find_child(n, keyAt(key, keyLen, depth));
    if (!child) {
        return NULL;
    }

    // If the child is leaf, delete from this node
    if (IS_LEAF(*child)) {
        artLeaf *l = LEAF_RAW(*child);
        if (leafNodeIsExactKey(l, key, keyLen)) {
            remove_child(n, ref, keyAt(key, keyLen, depth), child);
            return l;
        }

        return NULL;
    }

    // Recurse
    return recursive_delete(*child, child, key, keyLen, depth + 1, desc);
}

/**
 * Deletes a value from the ART tree
 * @arg t The tree
 * @arg key The key
 * @arg keyLen The length of the key
 * @return NULL if the item was not found, otherwise return value pointer.
 */
bool artDelete(art *t, const void *restrict const key,
               const uint_fast32_t keyLen, void **value) {
    artLeaf *l = recursive_delete(t->root, &t->root, key, keyLen, 0,
                                  ART_INCREMENT_REPLACE);
    if (l) {
        t->count--;

        if (value) {
            *value = l->value.ptr;
        }

        free(l);

        return true;
    }

    return false;
}

bool artDeleteDecrement(art *t, const void *key, uint_fast32_t keyLen,
                        const artIncrementDesc desc) {
    artLeaf *l = recursive_delete(t->root, &t->root, key, keyLen, 0, desc);
    if (l) {
        t->count--;
        free(l);

        /* Return 'true' meaning key was actually deleted */
        return true;
    }

    /* Return 'false' meaning key isn't deleted because it still has a count. */
    return false;
}

// Recursively iterates over the tree
static int recursive_iter(artNode *n, artCallback cb, void *data) {
    // Handle base cases
    if (!n) {
        return 0;
    }

    if (IS_LEAF(n)) {
        artLeaf *l = LEAF_RAW(n);
        return cb(data, l->key, l->keyLen, l->value.ptr);
    }

    int res;
    switch (n->type) {
    case NODE4:
        for (int i = 0; i < n->childrenCount; i++) {
            res = recursive_iter(((artNode4 *)n)->children[i], cb, data);
            if (res) {
                return res;
            }
        }

        break;
    case NODE16:
        for (int i = 0; i < n->childrenCount; i++) {
            res = recursive_iter(((artNode16 *)n)->children[i], cb, data);
            if (res) {
                return res;
            }
        }

        break;
    case NODE48:
        for (int i = 0; i < 256; i++) {
            const uint_fast8_t idx = ((artNode48 *)n)->keys[i];
            if (!idx) {
                continue;
            }

            res = recursive_iter(((artNode48 *)n)->children[idx - 1], cb, data);
            if (res) {
                return res;
            }
        }

        break;
    case NODE256:
        for (int i = 0; i < 256; i++) {
            /* The child nodes are not populated from low to high, so it's
             * possible to have a few dozen NULL nodes at the beginning then
             * catch populated entries towards the end.
             * The iterated node counts here do not respect n->childrenCount
             * and can exceed the count by design. */

            if (!((artNode256 *)n)->children[i]) {
                continue;
            }

            res = recursive_iter(((artNode256 *)n)->children[i], cb, data);
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
 * invoking a callback for each. The callback gets a
 * key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @arg t The tree to iterate over
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int artIter(art *t, artCallback cb, void *data) {
    return recursive_iter(t->root, cb, data);
}

/**
 * Checks if a leaf prefix matches
 * @return true on success.
 */
static bool leafPrefix_matches(const artLeaf *n, const void *prefix,
                               int prefixLen) {
    // Fail if the key length is too short
    if (n->keyLen < (uint32_t)prefixLen) {
        return false;
    }

    // Compare the keys
    return memcmp(n->key, prefix, prefixLen) == 0;
}

/**
 * Iterates through the entries pairs in the map,
 * invoking a callback for each that matches a given prefix.
 * The callback gets a key, value for each and returns an integer stop value.
 * If the callback returns non-zero, then the iteration stops.
 * @arg t The tree to iterate over
 * @arg key_ The prefix of keys to read
 * @arg keyLen The length of the prefix
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int artIterPrefix(const art *t, const void *key_, const uint_fast32_t keyLen,
                  artCallback cb, void *data) {
    artNode **child;
    artNode *n = t->root;
    int prefixLen;
    int depth = 0;
    const uint8_t *restrict key = key_;
    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = (artNode *)LEAF_RAW(n);
            // Check if the expanded path matches
            if (leafPrefix_matches((artLeaf *)n, key, keyLen)) {
                artLeaf *l = (artLeaf *)n;
                return cb(data, l->key, l->keyLen, l->value.ptr);
            }

            return 0;
        }

        // If the depth matches the prefix, we need to handle this node
        if (depth == keyLen) {
            artLeaf *l = minimum(n);
            if (leafPrefix_matches(l, key, keyLen)) {
                return recursive_iter(n, cb, data);
            }

            return 0;
        }

        // Bail if the prefix does not match
        if (n->partialLen) {
            prefixLen = prefix_mismatch(n, key, keyLen, depth);

            // Guard if the mis-match is longer than the MAX_PREFIX_LEN
            if ((uint32_t)prefixLen > n->partialLen) {
                prefixLen = n->partialLen;
            }

            // If there is no match, search is terminated
            if (!prefixLen) {
                return 0;
            }

            // If we've matched the prefix, iterate on this node
            if (depth + prefixLen == keyLen) {
                return recursive_iter(n, cb, data);
            }

            // if there is a full match, go deeper
            depth = depth + n->partialLen;
        }

#if 1
        if (depth >= keyLen) {
            // Node depth is greater than input key; can't match
            return 0;
        }
#endif

        // Recursively search
        child = find_child(n, keyAt(key, keyLen, depth));
        n = (child) ? *child : NULL;
        depth++;
    }

    return 0;
}

/* Copyright (c) 2012, Armon Dadgar
 * All rights reserved.
 *
 * Major changes and refactoring/improvements (c) 2018-2019, Matt Stancliff.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the organization nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL ARMON DADGAR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
