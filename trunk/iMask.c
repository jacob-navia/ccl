#include "containers.h"
#include "ccl_internal.h"

Mask *CreateFromMask(size_t n,char *data)
{
	Mask *result = CurrentMemoryManager->malloc(n+sizeof(Mask));
	if (result == NULL) {
		iError.RaiseError("iMask.Copy",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memcpy(result->data,data,n);
	result->Allocator = CurrentMemoryManager;
	result->length = n;
	return result;
}

Mask *Copy(Mask *src)
{
	if (src == NULL) return NULL;
	Mask *result = src->Allocator->malloc(src->length+sizeof(Mask));
	if (result == NULL) {
		iError.RaiseError("iMask.Copy",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memcpy(result,src,src->length+sizeof(Mask));
	result->Allocator = src->Allocator;
	return result;
}

Mask *Create(size_t n)
{
	Mask *result = CurrentMemoryManager->malloc(n+sizeof(Mask));
	if (result == NULL) {
		iError.RaiseError("iMask.Copy",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	result->Allocator = CurrentMemoryManager;
	memset(result->data,0,n);
	result->length = n;
	return result;
}

int Set(Mask *m,size_t idx,int val)
{
	if (idx >= m->length) {
		iError.RaiseError("iMask.Set",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	m->data[idx]=(char)val;
	return 1;
}

int Clear(Mask *m)
{
	memset(m->data,0,m->length);
	return 1;
}

int Finalize(Mask *m)
{
	m->Allocator->free(m);
	return 1;
}

size_t Size(Mask *m)
{
	return (m == NULL) ? 0 : m->length;
}

static int And(Mask *src1,Mask *src2)
{
	size_t i;

	if (src1 == NULL || src2 == NULL) {
		iError.RaiseError("iMask.And",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (src1->length != src2->length) {
		iError.RaiseError("iMask.And",CONTAINER_ERROR_INCOMPATIBLE);
		return CONTAINER_ERROR_INCOMPATIBLE;
	}
	for (i=0; i<src1->length;i++) {
		src1->data[i] &= src2->data[i];
	}
	return 1;
}

static int Not(Mask *src1)
{
        size_t i;

        if (src1 == NULL ) {
                iError.RaiseError("iMask.And",CONTAINER_ERROR_BADARG);
                return CONTAINER_ERROR_BADARG;
        }
        for (i=0; i<src1->length;i++) {
                src1->data[i] = ~src1->data[i];
        }
        return 1;
}


static int Or(Mask *src1,Mask *src2)
{
        size_t i;

        if (src1 == NULL || src2 == NULL) {
                iError.RaiseError("iMask.And",CONTAINER_ERROR_BADARG);
                return CONTAINER_ERROR_BADARG;
        }
        if (src1->length != src2->length) {
                iError.RaiseError("iMask.And",CONTAINER_ERROR_INCOMPATIBLE);
                return CONTAINER_ERROR_INCOMPATIBLE;
        }
        for (i=0; i<src1->length;i++) {
                src1->data[i] |= src2->data[i];
        }
        return 1;
}



MaskInterface iMask = {
	And,
	Or,
	Not,
	CreateFromMask,
	Create,
	Copy,
	Size,
	Set,
	Clear,
	Finalize
};
