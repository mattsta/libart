#pragma once

#include "artCommon.h"
#include <stddef.h> /* size_t */
#include <stdint.h>
__BEGIN_DECLS

typedef struct art art;
typedef struct artLeaf artLeaf;

art *artCreate(void);
void artFree(art *t);

void artInit(art *t);
void artFreeInner(art *t);

size_t artBytes(const art *t);
size_t artNodes(const art *t);
uint64_t artSize(const art *t);

void *artInsert(art *t, const void *key, int keyLen, void *value);
void *artDelete(art *t, const void *key, int keyLen);
void *artSearch(const art *t, const void *key, int keyLen);

void *artLeafValue(artLeaf *l);
void artLeafKey(artLeaf *l, void **key, size_t *len);

artLeaf *artMinimum(art *t);
artLeaf *artMaximum(art *t);

int artIter(art *t, artCallback cb, void *data);
int artIterPrefix(art *t, const void *prefix, int prefixLen, artCallback cb,
                  void *data);

__END_DECLS
