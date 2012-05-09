/*
  This is a thin layer based on malloc/free. It checks for
  (1) freeing a bloack that wasn't allocated with this utility
  (2) Freeing twice a block
  (3) Freeing a block that contains a memory overwrite past its allocated limit
  Besides this, it contains an API for finding out the size of a block
  Block layout:
  |----------|-----------|--------------- ... ------------------|-------|
  |Signature |   Size    |     user memory of requested size    | MAGIC |
  +----------+-----------+--------------------------------------+-------|
   lower addresses                               higher addresses
*/
#include "containers.h"
#define SIGNATURE 0xdeadbeef
#define MAGIC	0xbeefdead
static size_t AllocatedMemory;
#define ALIGN(size, boundary) (((size) + ((boundary) - 1)) & ~((boundary) - 1))
#define ALIGN_DEFAULT(size) ALIGN(size,sizeof(size_t))
static void *Malloc(size_t size)
{
	register char *r;
	register size_t *ip = NULL;
	size = ALIGN_DEFAULT(size);
	size += 3 * sizeof(size_t);
	r = malloc(size);
	if (r == NULL)
		return NULL;
	AllocatedMemory += size;
	ip = (size_t *) r;
	*ip++ = SIGNATURE;
	*ip++ = size;
	memset(ip, 0, size - 3*sizeof(size_t));
	ip = (size_t *) (&r[size - sizeof(size_t)]);
	*ip = MAGIC;
	return (r + 2 * sizeof(size_t));
}
static void Free(void *pp)
{
	size_t *ip = NULL;
	size_t s;
	register char *p = pp;
	if (p == NULL)
		return;
	p -= 2 * sizeof(size_t);
	ip = (size_t *) p;
	if (*ip == SIGNATURE) {
		*ip++ = 0;
		s = *ip;
		ip = (size_t *) (&p[s - sizeof(size_t)]);
		if (*ip != MAGIC) {
			/* overwritten block size %d\n */
			iError.RaiseError("Free",CONTAINER_ERROR_BUFFEROVERFLOW);
			return;
		}
		*ip = 0;
		AllocatedMemory -= s;
		memset(p,66,s);
		free(p);
	}
	else {
		/* Wrong block passed to Free */
		iError.RaiseError("Free",CONTAINER_ERROR_BADPOINTER);
	}
}

static void *Realloc(void *ptr,size_t newsize)
{
	size_t oldsize;
	size_t *ip;
	void *result;

	if (ptr == NULL)
		return Malloc(newsize);
	ip = ptr;
	oldsize = *--ip;
	if (*--ip != SIGNATURE) {
		/* Wrong block passed to Free */
        	iError.RaiseError("Realloc",CONTAINER_ERROR_BADPOINTER);
        	return NULL;
	}
	newsize = ALIGN_DEFAULT(newsize);
	oldsize -= 3*sizeof(size_t);
	if (oldsize == newsize)
		return ptr;
	ip++;
	if (newsize < oldsize) {
		char *p;
		*ip++ = newsize;
		p = (char *)ip;
		p += newsize - sizeof(size_t);
		ip = (size_t *)p;
		*ip = MAGIC;
		return ptr;
	}
	result = Malloc(newsize);
	if (result) {
		memcpy(result,ptr,oldsize);
		Free(ptr);
	}
	return result;
}

static void *Calloc(size_t n,size_t siz)
{
	size_t totalSize = n * siz;
	return Malloc(totalSize);
}

ContainerAllocator iDebugMalloc = {
Malloc,
Free,
Realloc,
Calloc,
};
