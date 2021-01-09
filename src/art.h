#pragma once

#include <stdbool.h>
#include <stddef.h> /* size_t */
#include <stdint.h>

#include "artCommon.h"

__BEGIN_DECLS

typedef struct art art;
typedef struct artLeaf artLeaf;

art *artNew(void);
void artFree(art *t);

void artInit(art *t);
void artFreeInner(art *t);

size_t artBytes(const art *t);
size_t artNodes(const art *t);
uint64_t artCount(const art *t);

bool artInsert(art *t, const void *key, uint_fast32_t keyLen, void *value,
               void **oldValue);
bool artInsertIncrement(art *t, const void *key, uint_fast32_t keyLen,
                        artIncrementDesc desc, artLeaf **usedLeaf);
bool artDelete(art *t, const void *key, uint_fast32_t keyLen, void **value);
bool artDeleteDecrement(art *t, const void *key, uint_fast32_t keyLen,
                        artIncrementDesc desc);
bool artSearch(const art *t, const void *key, uint_fast32_t keyLen,
               void **value);

void *artLeafValue(artLeaf *l);
size_t artLeafKey(artLeaf *l, void **key);
void *artLeafKeyOnly(artLeaf *l);
void artLeafIncrement(artLeaf *l);

artLeaf *artMinimum(art *t);
artLeaf *artMaximum(art *t);

int artIter(art *t, artCallback cb, void *data);
int artIterPrefix(const art *t, const void *prefix, uint_fast32_t prefixLen,
                  artCallback cb, void *data);

__END_DECLS
