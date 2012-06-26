#include "containers.h"
#include "ccl_internal.h"

static Mask *CreateFromMask(size_t n,char *data)
{
    Mask *result = CurrentAllocator->malloc(n+sizeof(Mask));
    if (result == NULL) {
        iError.RaiseError("iMask.Copy",CONTAINER_ERROR_NOMEMORY);
        return NULL;
    }
    if (data) memcpy(result->data,data,n);
    else memset(result->data,0,n);
    result->Allocator = CurrentAllocator;
    result->length = n;
    return result;
}

static Mask *Copy(Mask *src)
{
    Mask *result;
    if (src == NULL) return NULL;
    result = CreateFromMask(src->length, src->data);
    if (result) {
        result->Allocator = src->Allocator;
    }
    return result;
}

static Mask *Create(size_t n)
{
    return CreateFromMask(n,NULL);
}

static int Set(Mask *m,size_t idx,int val)
{
    if (idx >= m->length) {
        iError.RaiseError("iMask.Set",CONTAINER_ERROR_INDEX,m,idx);
        return CONTAINER_ERROR_INDEX;
    }
    m->data[idx]=(char)val;
    return 1;
}

static int Clear(Mask *m)
{
    memset(m->data,0,m->length);
    m->length=0;
    return 1;
}

static int Finalize(Mask *m)
{
    m->Allocator->free(m);
    return 1;
}

static size_t Size(Mask *m)
{
    return (m == NULL) ? 0 : m->length;
}

static int Verify(const Mask *src1, const Mask *src2,const char *name)
{
    if (src1 == NULL || src2 == NULL) {
        return iError.NullPtrError(name);
    }
    if (src1->length != src2->length) {
        iError.RaiseError(name,CONTAINER_ERROR_INCOMPATIBLE);
        return CONTAINER_ERROR_INCOMPATIBLE;
    }
    return 0;
}

static int And(Mask *src1,Mask *src2)
{
    size_t i;
    int r = Verify(src1,src2,"iMask.And");

    if (r) return r;
    for (i=0; i<src1->length;i++) {
        src1->data[i] &= src2->data[i];
    }
    return 1;
}

static int Not(Mask *src1)
{
    size_t i;

    if (src1 == NULL ) {
        return iError.NullPtrError("iMask.And");
    }
    for (i=0; i<src1->length;i++) {
        src1->data[i] = src1->data[i] ? 0 : 1;
    }
    return 1;
}


static int Or(Mask *src1,Mask *src2)
{
    size_t i;
    int r = Verify(src1,src2,"iMask.or");

    if (r) return r;
    for (i=0; i<src1->length;i++) {
            src1->data[i] |= src2->data[i];
    }
    return 1;
}

static size_t Sizeof(Mask *m)
{
    if (m == NULL) return sizeof(Mask);
    return sizeof(Mask) + m->length;
}

static size_t PopulationCount(const Mask *m)
{
    size_t i,result=0;

    for (i=0; i<m->length;i++) {
        if (m->data[i]) result++;
    }
    return result;
}

MaskInterface iMask = {
    And,
    Or,
    Not,
    CreateFromMask,
    Create,
    Copy,
    Size,
    Sizeof,
    Set,
    Clear,
    Finalize,
    PopulationCount,
};
