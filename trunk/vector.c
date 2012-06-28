#include "containers.h"
#include "ccl_internal.h"
#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif

static const guid VectorGuid = {0xba53f11e, 0x5879, 0x49e5,
{0x9e,0x3a,0xea,0x7d,0xd8,0xcb,0xd9,0xd6}
};

static int NullPtrError(const char *fnName)
{
	char buf[512];

	snprintf(buf,sizeof(buf),"iVector.%s",fnName);
	return iError.NullPtrError(buf);
}

static int doerror(const Vector *AL,const char *fnName,int code)
{
        char buf[512];

        (void)snprintf(buf,sizeof(buf),"iVector.%s",fnName);
        AL->RaiseError(buf,code);
        return code;
}

static int ErrorReadOnly(const Vector *AL,const char *fnName)
{
	return doerror(AL,fnName,CONTAINER_ERROR_READONLY);
}

static int ErrorIncompatible(const Vector *AL,const char *fnName)
{
    return doerror(AL,fnName,CONTAINER_ERROR_INCOMPATIBLE);
}

static int NoMemory(const Vector *AL,const char *fnName)
{
    return doerror(AL,fnName,CONTAINER_ERROR_NOMEMORY);
}

static Vector *Create(size_t elementsize,size_t startsize);

static void *DuplicateElement(const Vector *AL,void *str,size_t size,const char *functionName)
{
	void *result;
	if (size == 0)
		return str;
	if (str == NULL) {
		NullPtrError((char *)functionName);
		return NULL;
	}
	result = AL->Allocator->malloc(size);
	if (result == NULL) {
		NoMemory(AL,(char *)functionName);
	}
	else memcpy(result,str,size);
	return result;
}

#define CHUNKSIZE 20
static int DefaultVectorCompareFunction(const void *left,const void *right,CompareInfo *ExtraArgs)
{
	size_t siz=((Vector *)ExtraArgs->ContainerLeft)->ElementSize;
	return memcmp(left,right,siz);
}

static size_t Size(const Vector *AL)
{
	if (AL == NULL) {
		NullPtrError("Size");
		return 0;
	}
	return AL->count;
}
static unsigned GetFlags(const Vector *AL)
{
	if (AL == NULL) {
		NullPtrError("GetFlags");
		return 0;
	}
	return AL->Flags;
}
static unsigned SetFlags(Vector *AL,unsigned newval)
{
	unsigned oldval;
	if (AL == NULL) {
		NullPtrError("SetFlags");
		return 0;
	}
	oldval = AL->Flags;
	AL->Flags = newval;
	return oldval;
}


static int grow(Vector *AL)
{
	size_t newcapacity;
	void **oldcontents;
	int r = 1;

	newcapacity = AL->capacity + 1+AL->capacity/4;
	oldcontents = (void **)AL->contents;
	AL->contents = AL->Allocator->realloc(AL->contents,newcapacity*AL->ElementSize);
	if (AL->contents == NULL) {
		NoMemory(AL,"Resize");
		AL->contents = oldcontents;
		r = CONTAINER_ERROR_NOMEMORY;
	}
	else {
		AL->capacity = newcapacity;
		AL->timestamp++;
	}
	return r;
}

static int ResizeTo(Vector *AL,size_t newcapacity)
{
	void *oldcontents;

	if (AL == NULL) {
		return NullPtrError("ResizeTo");
	}
	if (AL->Flags & CONTAINER_READONLY)
		return ErrorReadOnly(AL,"ResizeTo");
	if (AL->capacity >= newcapacity)
		return 0;
	if (newcapacity <= AL->count)
		return 0;
	oldcontents = AL->contents;
	AL->contents = AL->Allocator->realloc(AL->contents,newcapacity*AL->ElementSize);
	if (AL->contents == NULL) {
		AL->contents = oldcontents;
		return NoMemory(AL,"ResizeTo");
	}
	AL->capacity = newcapacity;
	AL->timestamp++;
	return 1;
}

static int Resize(Vector *AL, size_t newSize)
{
	char *p;
	size_t i;
	if (AL == NULL) return iError.NullPtrError("iVector.Resize");
	if (AL->count < newSize) return ResizeTo(AL,newSize);
	p = (char *)AL->contents;
	if (AL->DestructorFn) {
		for (i=newSize; i<AL->count; i++) {
			AL->DestructorFn(p + i*AL->ElementSize);
		}
	}
	p = (char *)AL->Allocator->realloc(AL->contents,newSize*AL->ElementSize);
	if (p == NULL) {
		iError.RaiseError("iVector.Resize",CONTAINER_ERROR_NOMEMORY);
		return CONTAINER_ERROR_NOMEMORY;
	}
	AL->count = newSize;
	AL->capacity = newSize;
	AL->contents = (newSize ? p : NULL);
	return 1;
}
/*------------------------------------------------------------------------
 Procedure:     Add ID:1
 Purpose:       Adds an item at the end of the Vector
 Input:         The Vector and the item to be added
 Output:        The number of items in the Vector or negative if
                error
 Errors:
------------------------------------------------------------------------*/
static int Add(Vector *AL,const void *newval)
{
	char *p;

	if (AL == NULL) {
		return NullPtrError("Add");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Add");
	}
	if (newval == NULL) {
		AL->RaiseError("iVector.Add",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (AL->count >= AL->capacity) {
		int r = grow(AL);
		if (r <= 0)
			return r;
	}
	p = (char *)AL->contents;
	p += AL->count*AL->ElementSize;
	memcpy(p,newval,AL->ElementSize);
	AL->timestamp++;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_ADD,newval,NULL);
	++AL->count;
	return 1;
}

/*------------------------------------------------------------------------
 Procedure:     AddRange ID:1
 Purpose:       Adds a range of data at the end of an existing
                arrraylist. 
 Input:         The Vector and the data to be added
 Output:        One (OK) or negative error code
 Errors:        Vector must be writable and data must be
                different of NULL
------------------------------------------------------------------------*/
static int AddRange(Vector * AL,size_t n,const void *data)
{
	unsigned char *p;
	size_t newcapacity;

	if (n == 0)
		return 1;
	if (AL == NULL) {
		return NullPtrError("AddRange");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"AddRange");
	}
	if (data == NULL) {
		AL->RaiseError("iVector.AddRange",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	newcapacity = AL->count+n;
	if (newcapacity >= AL->capacity-1) {
		unsigned char *newcontents;
		newcapacity += AL->count/4;
		newcontents = (unsigned char *)AL->Allocator->realloc(AL->contents,newcapacity*AL->ElementSize);
		if (newcontents == NULL) {
			return NoMemory(AL,"AddRange");
		}
		AL->capacity = newcapacity;
		AL->contents = newcontents;
	}
	p = (unsigned char *)AL->contents;
	p += AL->count*AL->ElementSize;
	memcpy(p,data,n*AL->ElementSize);
	AL->count += n;
	AL->timestamp++;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_ADDRANGE,(void *)n,data);

	return 1;
}

static Vector *GetRange(const Vector *AL, size_t start,size_t end)
{
	Vector *result=NULL;
	char *p;
	size_t top;
	
	if (AL == NULL) {
		iError.RaiseError("iVector.GetRange",CONTAINER_ERROR_BADARG);
		return result;
	}
	if (AL->count == 0)
		return result;
	if (end >= AL->count)
		end = AL->count-1;
	if (start > end)
		return result;
	top = end-start;
	result = AL->VTable->Create(AL->ElementSize,top);
	if (result == NULL) {
		NoMemory(AL,"GetRange");
		return NULL;
	}
	p = (char *)AL->contents;
	memcpy(result->contents,p+start*AL->ElementSize,(top)*AL->ElementSize);
	result->count = end-start;
	return result;
}

/*------------------------------------------------------------------------
 Procedure:     Clear ID:1
 Purpose:       Frees all memory from an array list object without
                freeing the object itself
 Input:         The array list to be cleared
 Output:        The number of objects that were in the array list
 Errors:        The array list must be writable
------------------------------------------------------------------------*/
static int Clear(Vector *AL)
{
	if (AL == NULL) {
		return NullPtrError("Clear");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Clear");
	}

	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_CLEAR,NULL,NULL);

	if (AL->DestructorFn) {
		size_t i;
		unsigned char *p = (unsigned char *)AL->contents;
		for(i=0; i<AL->count;i++) {
			AL->DestructorFn(p);
			p += AL->ElementSize;
		}
	}
	AL->count = 0;
	AL->timestamp = 0;
	AL->Flags = 0;

	return 1;
}

static int Contains(const Vector *AL,const void *data,void *ExtraArgs)
{
	size_t i;
	char *p;
	CompareInfo ci;

	if (AL == NULL) {
		return NullPtrError("Contains");
	}
	if (data == NULL) {
		AL->RaiseError("iVector.Contains",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	p = (char *)AL->contents;
	if (ExtraArgs == NULL) {
		ExtraArgs = &ci;
		ci.ContainerLeft = AL;
		ci.ContainerRight = NULL;
		ci.ExtraArgs = ExtraArgs;
	}
	for (i = 0; i<AL->count;i++) {
		if (!AL->CompareFn(p,data,&ci))
			return 1;
		p += AL->ElementSize;
	}
	return 0;
}

static int Equal(const Vector *AL1,const Vector *AL2)
{
	size_t i;
	unsigned char *left,*right;
	if (AL1 == AL2)
		return 1;
	if (AL1 == NULL || AL2 == NULL)
		return 0;
	if (AL1->count != AL2->count)
		return 0;
	if (AL1->Flags != AL2->Flags)
		return 0;
	if (AL1->ElementSize != AL2->ElementSize)
		return 0;
	if (AL1->count == 0)
		return 1;
	if (AL1->CompareFn != AL2->CompareFn)
		return 0;
	if (AL1->CompareFn == DefaultVectorCompareFunction) {
		if (memcmp(AL1->contents,AL2->contents,AL1->ElementSize*AL1->count) != 0)
		return 0;
	}
	left =  (unsigned char *)AL1->contents;
	right = (unsigned char *)AL2->contents;
	for (i=0; i<AL1->count;i++) {
		if (AL1->CompareFn(left,right,NULL) != 0)
			return 0;
		left += AL1->ElementSize;
		right += AL2->ElementSize;
	}
	return 1;
}

static Vector *Copy(const Vector *AL)
{
	Vector *result;
	size_t startsize,es;
	
	if (AL == NULL) {
		NullPtrError("Copy");
		return NULL;
	}

	result = AL->Allocator->malloc(sizeof(*result));
	if (result == NULL) {
		NoMemory(AL,"Copy");
		return NULL;
	}
	memset(result,0,sizeof(*result));
	startsize = AL->count;
	if (startsize == 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * AL->ElementSize;
	result->contents = AL->Allocator->malloc(es);
	if (result->contents == NULL) {
		NoMemory(AL,"Copy");
		AL->Allocator->free(result);
		return NULL;
	}
	else {
		memset(result->contents,0,es);
		result->capacity = startsize;
		result->VTable = &iVector;
		result->ElementSize = AL->ElementSize;
		result->CompareFn = DefaultVectorCompareFunction;
		result->RaiseError = iError.RaiseError;
		result->Allocator = AL->Allocator;
	}

	memcpy(result->contents,AL->contents,AL->count*AL->ElementSize);
	result->CompareFn = AL->CompareFn;
	result->Flags = AL->Flags;
	result->RaiseError = AL->RaiseError;
	result->VTable = AL->VTable;
	result->count = AL->count;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_COPY,result,NULL);
	return result;
}

static int CopyElement(const Vector *AL,size_t idx, void *outbuf)
{
	char *p;
	if (AL == NULL || outbuf == NULL) {
		if (AL)
			AL->RaiseError("iVector.CopyElement",CONTAINER_ERROR_BADARG);
		else
			NullPtrError("CopyElement");
		return CONTAINER_ERROR_BADARG;
	}
	if (idx >= AL->count) {
		void *p=AL->RaiseError("iVector.CopyElement",CONTAINER_ERROR_INDEX,AL,idx);
		if (p) {
			// User overrides the error furnishing a pointer to some 
			// data. This allows implementing infinite arrays
			memcpy(outbuf,p,AL->ElementSize);
			return 1;
		}
		return CONTAINER_ERROR_INDEX;
	}
	p = (char *)AL->contents;
	p += (idx)*AL->ElementSize;
	memcpy(outbuf,p,AL->ElementSize);
	return 1;
}

static void **CopyTo(const Vector *AL)
{
	void **result;
	size_t i;
	char *p;
		
	if (AL == NULL) {
		NullPtrError("CopyTo");
		return NULL;
	}
	result = AL->Allocator->malloc((1+AL->count)*sizeof(void *));
	if (result == NULL) {
		NoMemory(AL,"CopyTo");
		return NULL;
	}
	p = AL->contents;
	for (i=0; i<AL->count;i++) {
		result[i] = DuplicateElement(AL,p,AL->ElementSize,"iVector.CopyTo");
		if (result[i] == NULL) {
			while (i > 0) {
				i--;
				AL->Allocator->free(result[i]);
			}
			AL->Allocator->free(result);
			return NULL;
		}
		p += AL->ElementSize;
	}
	result[i] = NULL;
	return result;
}

static int IndexOf(const Vector *AL,const void *data,void *ExtraArgs,size_t *result)
{
	size_t i;
	char *p;
	CompareInfo ci;

	if (AL == NULL) {
		return NullPtrError("IndexOf");
	}
	if (data == NULL) {
		AL->RaiseError("iVector.IndexOf",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	p = AL->contents;
	ci.ContainerLeft = (Vector *)AL;
	ci.ContainerRight = NULL;
	ci.ExtraArgs = ExtraArgs;
	for (i=0; i<AL->count;i++) {
		if (!AL->CompareFn(p,data,&ci)) {
			*result = i;
			return 1;
		}
		p += AL->ElementSize;
	}
	return CONTAINER_ERROR_NOTFOUND;
}

static void *GetElement(const Vector *AL,size_t idx)
{
	char *p;
	if (AL == NULL) {
		NullPtrError("GetElement");
		return NULL;
	}
	if (AL->Flags&CONTAINER_READONLY) {
		ErrorReadOnly(AL,"GetElement");
		return NULL;
	}
	if (idx >=AL->count ) {
		return  AL->RaiseError("iVector.GetElement",CONTAINER_ERROR_INDEX,AL,idx);
	}
	p = AL->contents;
	p += idx*AL->ElementSize;
	return p;
}

/*------------------------------------------------------------------------
 Procedure:     InsertAt ID:1
 Purpose:       Inserts data at the given position, that should be
                between zero and the number of items in the array
                list. If the index is equal to the count of items in
                the array the item will be inserted at the end. Any
                insertion in the middle or at the beginning provokes
                a move of all items between the index and the end of
                the array list
 Input:         The array list, the index and the new data to be
                inserted
 Output:        Error code less than zero if error, or positive
                number bigger than zero otherwise
 Errors:        The index must be correct and the array list must be
                writable
------------------------------------------------------------------------*/
static int InsertAt(Vector *AL,size_t idx,void *newval)
{
	char *p;
	
	if (AL == NULL) {
		return NullPtrError("InsertAt");
	}
	if (newval == NULL) {
		AL->RaiseError("iVector.InsertAt",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"InsertAt");
	}
	if (idx > AL->count) {
		p = AL->RaiseError("iVector.InsertAt",CONTAINER_ERROR_INDEX,AL,idx);
		if (p) {
			int r = ResizeTo(AL,idx+1);
			if (r < 0) return r;
		}
		else return CONTAINER_ERROR_INDEX;
	}
	if (AL->count >= (AL->capacity-1)) {
		int r = grow(AL);
		if (r <= 0)
			return r;
	}
	p = (char *)AL->contents;
	if (idx < AL->count) {
		memmove(p+AL->ElementSize*(idx+1),
				p+AL->ElementSize*idx,
				(AL->count-idx+1)*AL->ElementSize);
	}
	p += idx*AL->ElementSize;
	memcpy(p,newval,AL->ElementSize);
	AL->timestamp++;
	++AL->count;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_INSERT,newval,(void *)idx);
	return 1;
}

static int Insert(Vector *AL,void *newval)
{
	return InsertAt(AL,0,newval);
}

static int InsertIn(Vector *AL, size_t idx, Vector *newData)
{
	size_t newCount;
	char *p;

	if (AL == NULL) {
		return NullPtrError("InsertIn");
	}
	if (newData == NULL) {
		AL->RaiseError("InsertIn",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (AL->Flags & CONTAINER_READONLY)
		return ErrorReadOnly(AL,"InsertIn");
	if (idx > AL->count) {
		AL->RaiseError("iVector.InsertIn",CONTAINER_ERROR_INDEX,AL,idx);
		return CONTAINER_ERROR_INDEX;
	}
	if (AL->ElementSize != newData->ElementSize) {
		return ErrorIncompatible(AL,"InsertIn");
	}
	newCount = AL->count + newData->count;
	if (newCount >= (AL->capacity-1)) {
		int r = ResizeTo(AL,newCount);
		if (r <= 0)
			return r;
	}
	p = (char *)AL->contents;
	if (idx < AL->count) {
		memmove(p+AL->ElementSize*(idx+newData->count),
				p+AL->ElementSize*idx,
				(AL->count-idx)*AL->ElementSize);
	}
	p += idx*AL->ElementSize;
	memcpy(p,newData->contents,newData->ElementSize*newData->count);
	AL->timestamp++;
	AL->count = newCount;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_INSERT_IN,newData,NULL);
	return 1;
}

static int EraseAt(Vector *AL,size_t idx)
{
	char *p;

	if (AL == NULL) {
		return NullPtrError("EraseAt");
	}
	if (idx >= AL->count) {
		AL->RaiseError("iVector.Erase",CONTAINER_ERROR_INDEX,AL,idx);
		return CONTAINER_ERROR_INDEX;
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Erase");
	}
	p = (char *)AL->contents;
	p += AL->ElementSize * idx;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_ERASE_AT,p,NULL);
	if (AL->DestructorFn) {
		AL->DestructorFn(p);
	}
	if (idx < (AL->count-1)) {
		memmove(p+AL->ElementSize*idx,p+AL->ElementSize*(idx+1),(AL->count-idx)*AL->ElementSize);
	}
	AL->count--;
	AL->timestamp++;
	return 1;
}

static int RemoveRange(Vector *AL,size_t start, size_t end)
{
	size_t i;
	char *p;

	if (AL == NULL)
		return NullPtrError("RemoveRange");
	if (AL->count == 0)
		return 0;
	if (end > AL->count)
		end = AL->count;
	if (start == end) return 0;
	if (start >= AL->count) {
		return 0;
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Erase");
	}
	p = AL->contents;
	if (AL->DestructorFn) {
		for (i=start; i<end; i++) {
			AL->DestructorFn(p);
			AL->Allocator->free(p);
			p += AL->ElementSize;
		}
	}
	else {
		for (i=start; i<end; i++) {
			AL->Allocator->free(p);
			p += AL->ElementSize;
                }
        }
        if (end < AL->count)
        	memmove(p+start, p+end, (AL->count-end)*AL->ElementSize);
        AL->count -= end - start;
        AL->timestamp++;
        return 1;
}


static int EraseInternal(Vector *AL,const void *elem,int all)
{
    size_t i;
    char *p;
    CompareInfo ci;
    int result = CONTAINER_ERROR_NOTFOUND;

    if (AL == NULL) {
            return NullPtrError("Erase");
    }
    if (elem == NULL) {
            AL->RaiseError("iVector.Erase",CONTAINER_ERROR_BADARG);
            return CONTAINER_ERROR_BADARG;
    }
    if (AL->Flags & CONTAINER_READONLY)
        return ErrorReadOnly(AL,"Erase");

restart:
    p = AL->contents;
    ci.ContainerLeft = AL;
    ci.ContainerRight = NULL;
    ci.ExtraArgs = NULL;
    for (i=0; i<AL->count;i++) {
        if (!AL->CompareFn(p,elem,&ci)) {
            if (i > 0) {
                p = GetElement(AL,--i);
                result = EraseAt(AL,i+1);
            } else {
                result = EraseAt(AL,0);
                if (result < 0 || all == 0) return result;
                if (all) goto restart;
            }
            if (all == 0) return result;
            result = 1;
        }
        p += AL->ElementSize;
    }
    return result;

}

static int Erase(Vector *AL, const void *elem)
{
    return EraseInternal(AL,elem,0);
}
static int EraseAll(Vector *AL, const void *elem)
{
    return EraseInternal(AL,elem,1);
}



static int PushBack(Vector *AL,const void *str)
{
	if (AL == NULL) {
		return NullPtrError("PushBack");
	}
	return InsertAt(AL,AL->count,(void *)str);
}

/*------------------------------------------------------------------------
 Procedure:     Pop ID:1
 Purpose:       Removes the last item of the list and returns its
                data to the caller. It is the responsability of the
                caller to free this memory. This is almost the same as
				the "Remove" function, with the only difference that the
				data is NOT freed but returned to the caller.
 Input:         The array list to be popped
 Output:        a pointer to the last item's data
 Errors:        If the list is read-only or empty returns NULL
------------------------------------------------------------------------*/
static int PopBack(Vector *AL,void *result)
{
    char *p;

    if (AL == NULL) {
        return CONTAINER_ERROR_BADARG;
	}
    if (AL->Flags&CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"PopBack");
	}
    if (AL->count == 0)
            return 0;
    if (result) {
        p = AL->contents;
        p += AL->ElementSize*(AL->count-1);
        memcpy(result,p,AL->ElementSize);
    }
    AL->count--;
    AL->timestamp++;
    return 1;
}

/*------------------------------------------------------------------------
 Procedure:     Finalize ID:1
 Purpose:       Releases all memory associated with an Vector,
                including the Vector structure itself
 Input:         The Vector to be destroyed
 Output:        The number of items that the Vector contained or
                zero if error
 Errors:        Input must be writable
------------------------------------------------------------------------*/
static int Finalize(Vector *AL)
{
	unsigned Flags;
	int result;

	if (AL == NULL)
		return CONTAINER_ERROR_BADARG;
	Flags = AL->Flags;
	result = Clear(AL);
	if (result < 0)
		return result;
	if (Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_FINALIZE,NULL,NULL);
	if (AL->VTable != &iVector)
		AL->Allocator->free(AL->VTable);
	AL->Allocator->free(AL->contents);
	AL->Allocator->free(AL);
	return result;
}

static const ContainerAllocator *GetAllocator(const Vector *AL)
{
	if (AL == NULL) {
		return NULL;
	}
	return AL->Allocator;
}

static size_t GetCapacity(const Vector *AL)
{
	if (AL == NULL) {
		NullPtrError("GetCapacity");
		return 0;
	}
	return AL->capacity;
}

static int Mismatch(Vector *a1, Vector *a2,size_t *mismatch)
{
	size_t siz,i;
	CompareInfo ci;
	char *p1,*p2;

	*mismatch = 0;
	if (a1 == a2)
		return 0;
	if (a1 == NULL || a2 == NULL || mismatch == NULL) {
		return NullPtrError("Mismatch");
	}
	if (a1->CompareFn != a2->CompareFn || a1->ElementSize  != a2->ElementSize)
		return CONTAINER_ERROR_INCOMPATIBLE;
	siz = a1->count;
	if (siz > a2->count)
		siz = a2->count;
	if (siz == 0)
		return 1;
	p1 = a1->contents;
	p2 = a2->contents;
	ci.ContainerLeft = a1;
	ci.ContainerRight = a2;
	ci.ExtraArgs = NULL;
	for (i=0;i<siz;i++) {
		if (a1->CompareFn(p1,p2,&ci) != 0) {
			*mismatch = i;
			return 1;
		}
		p1 += a1->ElementSize;
		p2 += a2->ElementSize;
	}
	*mismatch = i;
	if (a1->count != a2->count)
		return 1;
	return 0;
}

static int SetCapacity(Vector *AL,size_t newCapacity)
{
	void **newContents;
	if (AL == NULL) {
		return NullPtrError("SetCapacity");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"SetCapacity");
	}
	newContents = AL->Allocator->malloc(newCapacity*AL->ElementSize);
	if (newContents == NULL) {
		return NoMemory(AL,"SetCapacity");
	}
	memset(AL->contents,0,AL->ElementSize*newCapacity);
	AL->capacity = newCapacity;
	if (newCapacity > AL->count)
		newCapacity = AL->count;
	else if (newCapacity < AL->count)
		AL->count = newCapacity;
	if (newCapacity > 0) {
		memcpy(newContents,AL->contents,newCapacity*AL->ElementSize);
	}
	AL->Allocator->free(AL->contents);
	AL->contents = newContents;
	AL->timestamp++;
	return 1;
}

static int Apply(Vector *AL,int (*Applyfn)(void *,void *),void *arg)
{
	size_t i;
	unsigned char *p,*pElem=NULL;

	if (AL == NULL || Applyfn == NULL) {
		return NullPtrError("Apply");
	}
	if (AL->Flags&CONTAINER_READONLY) {
		pElem = AL->Allocator->malloc(AL->ElementSize);
		if (pElem == NULL) {
			return NoMemory(AL,"Apply");
		}
	}
	p=AL->contents;
	for (i=0; i<AL->count;i++) {
		if (AL->Flags&CONTAINER_READONLY) {
			memcpy(pElem,p,AL->ElementSize);
			Applyfn(pElem,arg);
		}
		else Applyfn(p,arg);
		p += AL->ElementSize;
	}
	if (pElem)
		AL->Allocator->free(pElem);
	return 1;
}

/*------------------------------------------------------------------------
 Procedure:     ReplaceAt ID:1
 Purpose:       Replace the data at the given position with new
                data.
 Input:         The list and the new data
 Output:        Returns the new data if OK, NULL if error.
 Errors:        Index error or read only errors are caught
------------------------------------------------------------------------*/
static int ReplaceAt(Vector *AL,size_t idx,void *newval)
{
	char *p;

	if (AL == NULL || newval == NULL) {
		if (AL)
			AL->RaiseError("iVector.ReplaceAt",CONTAINER_ERROR_BADARG);
		else
			NullPtrError("ReplaceAt");
		return CONTAINER_ERROR_BADARG;
	}
	if (AL->Flags & CONTAINER_READONLY) {
		AL->RaiseError("iVector.ReplaceAt",CONTAINER_ERROR_READONLY);
		return CONTAINER_ERROR_READONLY;
	}
	if (idx >= AL->count) {
		AL->RaiseError("iVector.ReplaceAt",CONTAINER_ERROR_INDEX,AL,idx);
		return CONTAINER_ERROR_INDEX;
	}
	p = AL->contents;
	p += idx*AL->ElementSize;
	if (AL->Flags & CONTAINER_HAS_OBSERVER) {
		iObserver.Notify(AL,CCL_REPLACEAT,p,newval);
	}
	if (AL->DestructorFn)
		AL->DestructorFn(p);
	memcpy(p,newval,AL->ElementSize);
	AL->timestamp++;
	return 1;
}

static Vector *IndexIn(Vector *SC,Vector *AL)
{
	Vector *result = NULL;
	size_t i,top,idx;
	char *p;
	int r;

	if (SC == NULL || AL == NULL) {
		NullPtrError("IndexIn");
		return NULL;
	}
	if (AL == NULL) {
		NullPtrError("IndexIn");
		return NULL;
	}
	if (iVector.GetElementSize(AL) != sizeof(size_t)) {
		ErrorIncompatible(SC,"IndexIn");
		return NULL;
	}
	top = iVector.Size(AL);
	result = iVector.Create(SC->ElementSize,top);
	for (i=0; i<top;i++) {
		idx = *(size_t *)iVector.GetElement(AL,i);
		p = (char *)GetElement(SC,idx);
		if (p == NULL)
			goto err;
		r = Add(result,p);
		if (r < 0) {
err:
			Finalize(result);
			return NULL;
		}
	}
	return result;
}
static ErrorFunction SetErrorFunction(Vector *AL,ErrorFunction fn)
{
	ErrorFunction old;

	if (AL == NULL) {
		return iError.RaiseError;
	}
	old = AL->RaiseError;
	if (fn) AL->RaiseError = fn;
	return old;
}

static size_t Sizeof(const Vector *AL)
{
	if (AL == NULL)
		return sizeof(Vector);
	return sizeof(Vector) + (AL->count * AL->ElementSize);
}

/*------------------------------------------------------------------------
 Procedure:     SetCompareFunction ID:1
 Purpose:       Defines the function to be used in comparison of
                Vector elements
 Input:         The Vector and the new comparison function. If the new
                comparison function is NULL no changes are done.
 Output:        Returns the old function
 Errors:        None
------------------------------------------------------------------------*/
static CompareFunction SetCompareFunction(Vector *l,CompareFunction fn)
{
	CompareFunction oldfn;
	
	if (l == NULL) {
		NullPtrError("SetCompareFunction");
		return NULL;
	}
	oldfn = l->CompareFn;
	if (fn != NULL) /* Treat NULL as an enquiry to get the compare function */
		l->CompareFn = fn;
	return oldfn;
}

static int Sort(Vector *AL)
{
	CompareInfo ci;

	if (AL == NULL) {
		return NullPtrError("Sort");
	}
	ci.ContainerLeft = AL;
	ci.ExtraArgs = NULL;
	qsortEx(AL->contents,AL->count,AL->ElementSize,AL->CompareFn,&ci);
	return 1;
}

/* Proposed by PWO 
*/
static int Append(Vector *AL1, Vector *AL2)
{
	size_t newCount;
	char *p;

	if (AL1 == NULL) {
		return NullPtrError("Append");
	}
	if (AL2 == NULL) {
		AL1->RaiseError("iArrayL.Append",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if ((AL1->Flags & CONTAINER_READONLY) || (AL2->Flags & CONTAINER_READONLY)) {
		return ErrorReadOnly(AL1,"Append");
	}
	if (AL2->ElementSize != AL1->ElementSize) {
		return ErrorIncompatible(AL1,"Append");
	}
	newCount = AL1->count + AL2->count;
	if (newCount >= (AL1->capacity-1)) {
		int r = ResizeTo(AL1,newCount);
		if (r <= 0)
			return r;
	}
	p = (char *)AL1->contents;
	p += AL1->count*AL1->ElementSize;
	memcpy(p,AL2->contents,AL2->ElementSize*AL2->count);
	AL1->timestamp++;
	AL1->count = newCount;
	if (AL1->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL1,CCL_APPEND,AL2,NULL);
	AL2->Allocator->free(AL2);
    return 1;
}

/* Proposed by PWO 
*/
static int Reverse(Vector *AL)
{
	size_t s;
	char *p, *q, *t;

	if (AL == NULL) {
		return NullPtrError("Reverse");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Reverse");
	}
	if (AL->count < 2)
		return 1;

	s = AL->ElementSize;
	p = AL->contents;
	q = p + s*(AL->count-1);
	t = malloc(s);
	if (t == NULL) {
		return NoMemory(AL,"Reverse");
	}
	while ( p < q ) {
		memcpy(t, p, s);
		memcpy(p, q, s);
		memcpy(q, t, s);
		p += s;
		q -= s;
	}
	free(t);
	AL->timestamp++;
	return 1;
}


/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */


static void *GetNext(Iterator *it)
{
	struct VectorIterator *ali = (struct VectorIterator *)it;
	Vector *AL;
	char *p;

	if (ali == NULL) {
		NullPtrError("GetNext");
		return NULL;
	}
	AL = ali->AL;
	if (ali->index >= AL->count-1)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		AL->RaiseError("GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	p = AL->contents;
	++ali->index;
	p += ali->index*AL->ElementSize;
	if (AL->Flags & CONTAINER_READONLY) {
		memcpy(ali->ElementBuffer,p,AL->ElementSize);
		p = ali->ElementBuffer;
	}
	ali->Current = p;
	return p;
}

static size_t GetPosition(Iterator *it)
{
	struct VectorIterator *ali = (struct VectorIterator *)it;
	return ali->index;
}

static void *Seek(Iterator *it,size_t idx)
{
        struct VectorIterator *ali = (struct VectorIterator *)it;
        Vector *AL;
        char *p;

        if (ali == NULL) {
                NullPtrError("GetNext");
                return NULL;
        }
        AL = ali->AL;
        if (idx >= AL->count)
                return NULL;
        if (ali->timestamp != AL->timestamp) {
                AL->RaiseError("GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
                return NULL;
        }
        p = AL->contents;
        ali->index = idx;
        p += ali->index*AL->ElementSize;
        if (AL->Flags & CONTAINER_READONLY) {
                memcpy(ali->ElementBuffer,p,AL->ElementSize);
                p = ali->ElementBuffer;
        }
        ali->Current = p;
        return p;
}

static void *GetPrevious(Iterator *it)
{
	struct VectorIterator *ali = (struct VectorIterator *)it;
	Vector *AL;
	char *p;

	if (ali == NULL) {
		NullPtrError("GetPrevious");
		return NULL;
	}
	AL = ali->AL;
	if (ali->index >= AL->count || ali->index == 0)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		AL->RaiseError("iVector.GetPrevious",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	p = AL->contents;
	ali->index--;
	p += ali->index*AL->ElementSize;
	if (AL->Flags & CONTAINER_READONLY) {
		memcpy(ali->ElementBuffer,p,AL->ElementSize);
		p = ali->ElementBuffer;
	}
	ali->Current = p;
	return p;
}

static void *GetLast(Iterator *it)
{
	struct VectorIterator *ali = (struct VectorIterator *)it;
	Vector *AL;
	char *p;

	if (ali == NULL) {
		NullPtrError("GetLast");
		return NULL;
	}
	AL = ali->AL;
	if (AL->count == 0)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		AL->RaiseError("iVector.GetLast",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	p = AL->contents;
	p += (AL->count-1) *AL->ElementSize;
	if (AL->Flags & CONTAINER_READONLY) {
		memcpy(ali->ElementBuffer,p,AL->ElementSize);
		p = ali->ElementBuffer;
	}
	ali->index = AL->count-1;
	ali->Current = p;
	return p;
}

static void *GetCurrent(Iterator *it)
{
	struct VectorIterator *ali = (struct VectorIterator *)it;
	if (ali == NULL) {
		NullPtrError("GetCurrent");
		return NULL;
	}
	return ali->Current;
}

static int ReplaceWithIterator(Iterator *it, void *data,int direction) 
{
    struct VectorIterator *ali = (struct VectorIterator *)it;
	int result;
	size_t pos;
	
	if (it == NULL) {
		return NullPtrError("Replace");
	}
	if (ali->AL->count == 0)
		return 0;
    if (ali->timestamp != ali->AL->timestamp) {
        ali->AL->RaiseError("Replace",CONTAINER_ERROR_OBJECT_CHANGED);
        return CONTAINER_ERROR_OBJECT_CHANGED;
    }
	if (ali->AL->Flags & CONTAINER_READONLY) {
		ali->AL->RaiseError("Replace",CONTAINER_ERROR_READONLY);
		return CONTAINER_ERROR_READONLY;
	}	
	pos = ali->index;
	if (direction)
		GetNext(it);
	else
		GetPrevious(it);
	if (data == NULL)
		result = EraseAt(ali->AL,pos);
	else {
		result = ReplaceAt(ali->AL,pos,data);
	}
	if (result >= 0) {
		ali->timestamp = ali->AL->timestamp;
	}
	return result;
}


static void *GetFirst(Iterator *it)
{
	struct VectorIterator *ali = (struct VectorIterator *)it;

	if (ali == NULL) {
		NullPtrError("GetFirst");
		return NULL;
	}
	if (ali->AL->count == 0) {
		ali->Current = NULL;
		return NULL;
	}
	ali->index = 1;
	ali->Current = ali->AL->contents;
	if (ali->AL->Flags & CONTAINER_READONLY) {
		memcpy(ali->ElementBuffer,ali->AL->contents,ali->AL->ElementSize);
		ali->Current = ali->ElementBuffer;
	}
	return ali->Current;
}

static Iterator *NewIterator(Vector *AL)
{
	struct VectorIterator *result;

	if (AL == NULL) {
		NullPtrError("NewIterator");
		return NULL;
	}
	result = AL->Allocator->calloc(sizeof(struct VectorIterator)+AL->ElementSize,1);
	if (result == NULL) {
		NoMemory(AL,"NewIterator");
		return NULL;
	}
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetPrevious;
	result->it.GetFirst = GetFirst;
	result->it.GetCurrent = GetCurrent;
	result->it.GetLast = GetLast;
	result->it.Seek = Seek;
	result->it.GetPosition = GetPosition;
	result->it.Replace = ReplaceWithIterator;
	result->AL = AL;
	result->Current = NULL;
	result->timestamp = AL->timestamp;
	result->index = -1;
	return &result->it;
}

static int InitIterator(Vector *AL,void *buf)
{
        struct VectorIterator *result;

        if (AL == NULL || buf == NULL) {
                NullPtrError("NewIterator");
                return CONTAINER_ERROR_BADARG;
        }
        result = buf;
        result->it.GetNext = GetNext;
        result->it.GetPrevious = GetPrevious;
        result->it.GetFirst = GetFirst;
        result->it.GetCurrent = GetCurrent;
        result->it.GetLast = GetLast;
        result->it.Seek = Seek;
        result->it.Replace = ReplaceWithIterator;
        result->AL = AL;
        result->Current = NULL;
        result->timestamp = AL->timestamp;
        result->index = -1;
        return 1;
}


static int deleteIterator(Iterator * it)
{
	struct VectorIterator *ali = (struct VectorIterator *)it;
	if (ali == NULL) {
		return NullPtrError("deleteIterator");
	}
	ali->AL->Allocator->free(it);
	return 1;
}

static int DefaultSaveFunction(const void *element,void *arg, FILE *Outfile)
{
	const unsigned char *str = element;
	size_t len = *(size_t *)arg;

	return len == fwrite(str,1,len,Outfile);
}

static int DefaultLoadFunction(void *element,void *arg, FILE *Infile)
{
	size_t len = *(size_t *)arg;

	return len == fread(element,1,len,Infile);
}

static int Save(const Vector *AL,FILE *stream, SaveFunction saveFn,void *arg)
{
	size_t i,elemsiz;

	if (AL == NULL) {
		return NullPtrError("Save");
	}
	if (stream == NULL) {
		AL->RaiseError("iVector.Save",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (saveFn == NULL) {
		saveFn = DefaultSaveFunction;
                elemsiz = AL->ElementSize;
		arg = &elemsiz;
	}
	if (fwrite(&VectorGuid,sizeof(guid),1,stream) == 0)
		return EOF;
	if (fwrite(AL,1,sizeof(Vector),stream) == 0)
		return EOF;
	for (i=0; i< AL->count; i++) {
		char *p = AL->contents;

		p += i*AL->ElementSize;
		if (saveFn(p,arg,stream) <= 0)
			return EOF;
	}
	return 1;
}

#if 0
static int SaveToBuffer(Vector *AL,char *stream, size_t *bufferlen,SaveFunction saveFn,void *arg)
{
	size_t i,len;
	int elemLen;
	
	if (AL == NULL) {
		return NullPtrError("iVector.SaveToBuffer");
	}
	if (bufferlen == NULL) {
		AL->RaiseError("iVector.SaveToBuffer",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (stream)
		len = *bufferlen;
	else {
		len = sizeof(guid) + sizeof(Vector);
	}
	if (saveFn == NULL) {
		saveFn = DefaultSaveFunction;
		arg = &AL->ElementSize;
	}
	*bufferlen = 0;
	if (stream) {
		if (len >= sizeof(guid)) {
			memcpy(stream,&VectorGuid,sizeof(guid));
			len -= sizeof(guid);
			stream += sizeof(guid);
		}
		else return EOF;
		if (len >= sizeof(Vector)) {
			memcpy(stream,AL,sizeof(Vector));
			len -= sizeof(Vector);
			stream += sizeof(Vector);
		}
		else return EOF;
	}
	for (i=0; i< AL->count; i++) {
		char *p = AL->contents;
		
		p += i*AL->ElementSize;
		elemLen = saveFn(p,arg,stream,len);
		if (elemLen<= 0)
			return EOF;
		if (stream) {
			if (len > elemLen)
				len -= elemLen;
			else return EOF;
		}
		else len += elemLen;
	}
	if (stream == NULL)
		*bufferlen = len;
	return 1;
}
#endif

static Vector *Load(FILE *stream, ReadFunction loadFn,void *arg)
{
	size_t i;
	unsigned char *p;
	Vector *result,AL;
	guid Guid;

	if (stream == NULL) {
		NullPtrError("Load");
		return NULL;
	}
	if (loadFn == NULL) {
		loadFn = DefaultLoadFunction;
		arg = &AL.ElementSize;
	}
	if (fread(&Guid,sizeof(guid),1,stream) == 0)
		return NULL;
	if (memcmp(&Guid,&VectorGuid,sizeof(guid))) {
		iError.RaiseError("iVector.Load",CONTAINER_ERROR_WRONGFILE);
		return NULL;
	}
	if (fread(&AL,1,sizeof(Vector),stream) == 0)
		return NULL;
	result = Create(AL.ElementSize,AL.count);
	if (result) {
		p = result->contents;
		for (i=0; i< AL.count; i++) {
			if (loadFn(p,arg,stream) <= 0) {
				iError.RaiseError("iVector.Load",CONTAINER_ERROR_FILE_READ);
				Finalize(result);
				return NULL;
			}
			p += result->ElementSize;
			result->count++;
		}
		result->Flags = AL.Flags;
	}
	else iError.RaiseError("iVector.Load",CONTAINER_ERROR_NOMEMORY);
	return result;
}



static Vector *CreateWithAllocator(size_t elementsize,size_t startsize,const ContainerAllocator *allocator)
{
	Vector *result;
	size_t es;
	
	/* Allocate space for the array list header structure */
	result = allocator->malloc(sizeof(*result));
	if (result == NULL) {
		iError.RaiseError("iVector.Create",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memset(result,0,sizeof(*result));
	if (startsize == 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * elementsize;
	result->contents = allocator->malloc(es);
	if (result->contents == NULL) {
		iError.RaiseError("iVector.Create",CONTAINER_ERROR_NOMEMORY);
		allocator->free(result);
		result = NULL;
	}
	else {
		memset(result->contents,0,es);
		result->capacity = startsize;
		result->VTable = &iVector;
		result->ElementSize = elementsize;
		result->CompareFn = DefaultVectorCompareFunction;
		result->RaiseError = iError.RaiseError;
		result->Allocator = (ContainerAllocator *)allocator;
	}
	return result;
}

static Vector *Create(size_t elementsize,size_t startsize)
{
	return CreateWithAllocator(elementsize,startsize,CurrentAllocator);
}

static Vector *InitializeWith(size_t elementSize,size_t n,const void *data)
{
        Vector *result = Create(elementSize,n);
        if (result == NULL)
                return result;
        memcpy(result->contents,data,n*elementSize);
        result->count = n;
        return result;
}


static int SearchWithKey(Vector *vec,size_t startByte,size_t sizeKey,size_t startidx,void *item,size_t *result)
{
	size_t i;
	char *p;

	if (vec == NULL || result == NULL) {
		return NullPtrError("SearchWithKey");
	}
	if (startByte >= vec->ElementSize) {
		return 0;
	}
	if (sizeKey >= (vec->ElementSize-startByte))
		sizeKey = vec->ElementSize-startByte;
	if (startidx >= vec->count)
		return 0;
	p = vec->contents;
	for (i=startidx; i<vec->count; i++) {
		if (memcmp(p+startByte,item,sizeKey) == 0) {
			*result = i;
			return 1;
		}
		p += vec->ElementSize;
	}
	return 0;
}

static Vector *Init(Vector *result,size_t elementsize,size_t startsize)
{
	size_t es;
	
	memset(result,0,sizeof(*result));
	if (startsize == 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * elementsize;
	result->contents = CurrentAllocator->malloc(es);
	if (result->contents == NULL) {
		iError.RaiseError("iVector.Init",CONTAINER_ERROR_NOMEMORY);
		result = NULL;
	}
	else {
		memset(result->contents,0,es);
		result->capacity = startsize;
		result->VTable = &iVector;
		result->ElementSize = elementsize;
		result->CompareFn = DefaultVectorCompareFunction;
		result->RaiseError = iError.RaiseError;
		result->Allocator = CurrentAllocator;
	}
	return result;
}

static size_t GetElementSize(const Vector *AL)
{
	if (AL == NULL) {
		NullPtrError("GetElementSize");
		return 0;
	}
	return AL->ElementSize;
}
static DestructorFunction SetDestructor(Vector *cb,DestructorFunction fn)
{
	DestructorFunction oldfn;
	if (cb == NULL)
		return NULL;
	oldfn = cb->DestructorFn;
	if (fn)
		cb->DestructorFn = fn;
	return oldfn;
}

static Vector * SelectCopy(Vector *src,Mask *m)
{
    size_t i,offset=0,siz;
    Vector *result;
	char *dst,*s;

    if (src == NULL || m == NULL) {
        NullPtrError("SelectCopy");
        return NULL;
    }
    if (m->length != src->count) {
        iError.RaiseError("SelectCopy",CONTAINER_ERROR_BADMASK,src,m);
        return NULL;
    }
    siz = src->ElementSize;
    result = Create(siz,src->count);
    dst = result->contents;
    s = src->contents;
    if (result == NULL) {
        NoMemory(src,"SelectCopy");
        return NULL;
    }
    for (i=0; i<m->length;i++) {
        if (m->data[i]) {
            if (i != offset)
               memcpy(dst+offset*siz , s+i*siz,siz);
            offset++;
        }
    }
    result->count = offset;
    return result;
}

static void **GetData(const Vector *cb)
{
	if (cb == NULL) {
		NullPtrError("GetData");
		return NULL;
	}
	if (cb->Flags&CONTAINER_READONLY) {
		cb->RaiseError("GetData",CONTAINER_ERROR_READONLY);
		return NULL;
	}
	return cb->contents;
}

static void *Back(const Vector *v)
{
	char *pdata;
	if (v == NULL) {
		NullPtrError("Back");
		return NULL;
	}
	if (v->Flags&CONTAINER_READONLY) {
		v->RaiseError("Back",CONTAINER_ERROR_READONLY);
		return NULL;
	}
	if (v->count == 0)
		return NULL;
	pdata = v->contents;
	pdata += v->ElementSize*(v->count-1);
	return pdata;
}
static void *Front(const Vector *v)
{
	if (v == NULL) {
		NullPtrError("Front");
		return NULL;
	}
	if (v->Flags&CONTAINER_READONLY) {
		v->RaiseError("Front",CONTAINER_ERROR_READONLY);
		return NULL;
	}
	if (v->count == 0)
		return NULL;
	return v->contents;
}

static size_t SizeofIterator(const Vector *v)
{
	return sizeof(struct VectorIterator)+v->ElementSize;
}

static int RotateLeft(Vector *AL, size_t n)
{
        char *p,*q,*t;

	if (AL == NULL) return NullPtrError("RotateLeft");
	if (AL->Flags & CONTAINER_READONLY)
		return ErrorReadOnly(AL,"RotateLeft");
	if (AL->count == 0 || n == 0) return 0;
        n %= AL->count;
        if (n == 0)
                return 1;
        /* Reverse the first partition */
	t = AL->Allocator->malloc(AL->ElementSize);
	if (t == NULL) {
		AL->RaiseError("RotateLeft",CONTAINER_ERROR_NOMEMORY);
		return CONTAINER_ERROR_NOMEMORY;
	}

        p = AL->contents;
        q = (char *)AL->contents+(n-1)*AL->ElementSize;
        while (p < q) {
		memcpy(t,p,AL->ElementSize);
		memcpy(p,q,AL->ElementSize);
		memcpy(q,t,AL->ElementSize);
		p += AL->ElementSize;
		q -= AL->ElementSize;
        }
        /* Reverse the second partition */
        p = AL->contents;
        p += n*AL->ElementSize;
        q = AL->contents;
        q+=AL->ElementSize*(AL->count-1);
        if (n < AL->count-1) {
                while (p<q) {
			memcpy(t,p,AL->ElementSize);
			memcpy(p,q,AL->ElementSize);
			memcpy(q,t,AL->ElementSize);
			p += AL->ElementSize;
			q -= AL->ElementSize;
                }
        }
        /* Now reverse the whole */
        p = AL->contents;
        q = p+AL->ElementSize*(AL->count-1);
        while (p<q) {
		memcpy(t,p,AL->ElementSize);
		memcpy(p,q,AL->ElementSize);
		memcpy(q,t,AL->ElementSize);
		p += AL->ElementSize;
		q -= AL->ElementSize;
        }
	AL->Allocator->free(t);
        return 1;
}

static int RotateRight(Vector *AL, size_t n)
{
        char *p,*q,*t;

	if (AL == NULL) return NullPtrError("RotateLeft");
	if (AL->Flags & CONTAINER_READONLY)
		return ErrorReadOnly(AL,"RotateRight");
	if (AL->count == 0 || n == 0) return 0;
        n %= AL->count;
        if (n == 0)
                return 1;
        /* Reverse the first partition */
	t = AL->Allocator->malloc(AL->ElementSize);
	if (t == NULL) {
		AL->RaiseError("RotateLeft",CONTAINER_ERROR_NOMEMORY);
		return CONTAINER_ERROR_NOMEMORY;
	}
        p = AL->contents;
        q = p+(AL->count-1)*AL->ElementSize;
	p = p + AL->ElementSize*(AL->count-n);
        while (p < q) {
		memcpy(t,p,AL->ElementSize);
		memcpy(p,q,AL->ElementSize);
		memcpy(q,t,AL->ElementSize);
		p += AL->ElementSize;
		q -= AL->ElementSize;
        }
        /* Reverse the second partition */
        p = AL->contents;
        q = p + AL->ElementSize*(AL->count-n-1);
        if (n < AL->count-1) {
                while (p<q) {
			memcpy(t,p,AL->ElementSize);
			memcpy(p,q,AL->ElementSize);
			memcpy(q,t,AL->ElementSize);
			p += AL->ElementSize;
			q -= AL->ElementSize;
                }
        }
        /* Now reverse the whole */
        p = AL->contents;
        q = p+AL->ElementSize*(AL->count-1);
        while (p<q) {
		memcpy(t,p,AL->ElementSize);
		memcpy(p,q,AL->ElementSize);
		memcpy(q,t,AL->ElementSize);
		p += AL->ElementSize;
		q -= AL->ElementSize;
        }
	AL->Allocator->free(t);
        return 1;
}

static Mask *CompareEqual(const Vector *left,const Vector *right,Mask *bytearray)
{
	
	size_t left_len;
	size_t right_len;
	size_t i;
	char *pleft,*pright;
	
	if (left == NULL || right == NULL) {
		NullPtrError("CompareEqual");
		return NULL;
	}
	left_len = left->count; right_len = right->count;
	if (left_len != right_len || left->ElementSize != right->ElementSize) {
		iError.RaiseError("iVector.CompareEqual",CONTAINER_ERROR_INCOMPATIBLE);
		return NULL;
	}
	if (bytearray == NULL || iMask.Size(bytearray) < left_len) {
		if (bytearray) iMask.Finalize(bytearray);
		bytearray = iMask.Create(left_len);
		if (bytearray == NULL) {
			NoMemory(left,"CompareEqual");
			return NULL;
		}
	}
	else iMask.Clear(bytearray);
	pleft = left->contents;
	pright = right->contents;
	for (i=0; i<left_len;i++) {
		bytearray->data[i] = !left->CompareFn(left,right,NULL);
		pleft += left->ElementSize;
		pright += right->ElementSize;
	}
	return bytearray;
}

static Mask *CompareEqualScalar(const Vector *left,const void *right,Mask *bytearray)
{
	size_t left_len;
	size_t i;
	char *pleft;
	
	if (left == NULL || right == NULL) {
		NullPtrError("CompareEqual");
		return NULL;
	}
	left_len = left->count;
	if (bytearray == NULL || iMask.Size(bytearray) < left_len) {
		if (bytearray) iMask.Finalize(bytearray);
		bytearray = iMask.Create(left_len);
	}
	else iMask.Clear(bytearray);
	if (bytearray == NULL) {
		NoMemory(left,"CompareEqual");
		return NULL;
	}
	pleft = left->contents;
	if (left->CompareFn == DefaultVectorCompareFunction) {
		for (i=0; i<left_len; i++) {
			bytearray->data[i] = !memcmp(left,right,left->ElementSize);
			pleft += left->ElementSize;
		}
	}
	else for (i=0; i<left_len;i++) {
		bytearray->data[i] = !left->CompareFn(left,right,NULL);
		pleft += left->ElementSize;
	}
	return bytearray;
}


static int Select(Vector *src,const Mask *m)
{
	size_t i,offset=0;
	char *p,*q;

	if (src == NULL || m == NULL)
		return NullPtrError("Select");
	if (m->length != src->count) {
		iError.RaiseError("iVector.Select",CONTAINER_ERROR_BADMASK,src,m);
		return CONTAINER_ERROR_BADMASK;
	}
	q = p = src->contents;
	for (i=0; i<m->length;i++) {
		if (m->data[i]) {
			if (i != offset) {
				if (src->DestructorFn)
					src->DestructorFn(p);
				memcpy(p , q, src->ElementSize);
				p += src->ElementSize;
			}
			offset++;
			q += src->ElementSize;
		}
	}
	if (offset < i) {
		memset((char *)(src->contents)+offset*src->ElementSize,0,src->ElementSize*(i-offset));
	}
	src->count = offset;
	return 1;
}

VectorInterface iVector = {
	Size,
	GetFlags, 
	SetFlags, 
	Clear,
	Contains,
	Erase, 
        EraseAll,
	Finalize,
	Apply,
	Equal,
	Copy,
	SetErrorFunction,
	Sizeof,
	NewIterator,
	InitIterator,
	deleteIterator,
	SizeofIterator,
	Save,
	Load,
	GetElementSize,
/* Sequential container fields */
	Add, 
	GetElement,
	PushBack,
	PopBack,
	InsertAt,
	EraseAt, 
	ReplaceAt,
	IndexOf, 
/* Vector specific fields */
	Insert,
	InsertIn,
	IndexIn,
	GetCapacity, 
	SetCapacity,
	SetCompareFunction,
	Sort,
	Create,
	CreateWithAllocator,
	Init,
	AddRange, 
	GetRange,
	CopyElement,
	CopyTo, 
	Reverse,
	Append,
	Mismatch,
	GetAllocator,
	SetDestructor,
	SearchWithKey,
	Select,
	SelectCopy,
	Resize,
	InitializeWith,
	GetData,
	Back,
	Front,
	RemoveRange,
	RotateLeft,
	RotateRight,
	CompareEqual,
	CompareEqualScalar,
	ResizeTo, // Reserve
};
