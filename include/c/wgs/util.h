#ifndef _wgs_util_
#define _wgs_util_

#include <wgs/obj.h>
#include <sys/types.h>

typedef struct JIter {
    JObj jobj;
} JIter;

typedef struct MJIter {
int (*HasNext)(JIter *Self);
void *(*Next)(JIter *Self);
} MJIter;

typedef struct Vec {
void **Ptrs;
uint size;
uint len;
} Vec;

Vec *VecInit(Vec *Self);
void VecEnsure(Vec *Self, uint len);
void VecAdd(Vec *Self, void *ptr);
void *VecGet(Vec *Self, uint i);
void VecSet(Vec *Self, uint i, void *ptr);
void VecRemove(Vec *Self, void *ptr);
void VecRemIndex(Vec *Self, uint i);
JIter *VecIter(Vec *Self);
#define VecSize(a) (a)->size
int VecIndexOf(Vec *Self, void *ptr);



JIter *JIterInit(JIter *Self);
JIter *JVIterInit(JIter *Self, Vec *Vec);
void *JIterNext(JIter *Self);
int JIterHasNext(JIter *Self);

#endif
