#include <wgs/util.h>
#include <stdlib.h>

Vec *VecInit(Vec *Self)
{
    Vec *ret = calloc(sizeof(Vec), 1);
    ret->Ptrs = malloc(8*sizeof(void *));
    ret->len = 8;
    ret->size = 0;
    return ret;
}

void VecEnsure(Vec *Self, uint len)
{
    if (len <= Self->len)
	return;
    Self->len *= 2;
    Self->Ptrs = realloc(Self->Ptrs, Self->len * sizeof(void *));
}

JIter *VecIter(Vec *Self)
{
    return JVIterInit(NULL, Self);
}

void VecAdd(Vec *Self, void *ptr)
{
    VecEnsure(Self, Self->size+1);
    Self->Ptrs[Self->size] = ptr;
    Self->size++;
}

void *VecGet(Vec *Self, uint i)
{
    return Self->Ptrs[i];
}

void VecSet(Vec *Self, uint i, void *ptr)
{
    Self->Ptrs[i] = ptr;
}

void VecRemove(Vec *Self, uint i)
{
    return;
}

int VecIndexOf(Vec *Self, void *ptr)
{
    uint i=0;
    uint len=Self->len;
    void **Ptrs = Self->Ptrs;
    for (i=0; i<len; i++)
    {
	if (ptr == Ptrs[i])
	    return i;
    }
    return -1;
}
