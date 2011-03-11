#include "containers.h"
#include "ccl_internal.h"
#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif
/* Definition of the String Collection type */
struct _Vector {
    VectorInterface *VTable; /* The table of functions */
    size_t count;                  /* number of elements in the array */
    unsigned int Flags;             /* Read-only or other flags */
    size_t ElementSize;		/* Size of the elements stored in this array. */
    void *contents;               /* The contents of the collection */
    size_t capacity;                /* allocated space in the contents vector */
	unsigned timestamp;
    /* Element comparison function */
	CompareFunction CompareFn;
    /* Error function */
    ErrorFunction RaiseError;
	ContainerMemoryManager *Allocator;
	DestructorFunction DestructorFn;
} ;

static const guid VectorGuid = {0xba53f11e, 0x5879, 0x49e5,
{0x9e,0x3a,0xea,0x7d,0xd8,0xcb,0xd9,0xd6}
};

static int ErrorReadOnly(Vector *AL,char *fnName)
{
	char buf[512];

	sprintf(buf,"iVector.%s",fnName);
	AL->RaiseError(buf,CONTAINER_ERROR_READONLY);
	return CONTAINER_ERROR_READONLY;
}

static int NullPtrError(char *fnName)
{
	char buf[512];

	sprintf(buf,"iVector.%s",fnName);
	iError.RaiseError(buf,CONTAINER_ERROR_BADARG);
	return CONTAINER_ERROR_BADARG;
}


static Vector *Create(size_t elementsize,size_t startsize);

static void *DuplicateElement(Vector *AL,void *str,size_t size,const char *functionName)
{
	void *result;
	if (size == 0)
		return str;
	if (str == NULL) {
		iError.RaiseError(functionName,CONTAINER_ERROR_BADARG);
		return NULL;
	}
	result = MALLOC(AL,size);
	if (result == NULL) {
		iError.RaiseError(functionName,CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	else memcpy(result,str,size);
	return result;
}

#define CHUNKSIZE 20
static int DefaultVectorCompareFunction(const void *left,const void *right,CompareInfo *ExtraArgs)
{
	size_t siz=((Vector *)ExtraArgs->Container)->ElementSize;
	return memcmp(left,right,siz);
}

static size_t GetCount(const Vector *AL)
{
	if (AL == NULL) {
		NullPtrError("GetCount");
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
	int oldval;
	if (AL == NULL) {
		NullPtrError("SetFlags");
		return 0;
	}
	oldval = AL->Flags;
	AL->Flags = newval;
	return oldval;
}


static int Resize(Vector *AL)
{
	size_t newcapacity;
	void **oldcontents;

	if (AL == NULL) {
		return NullPtrError("Resize");
	}
	newcapacity = AL->capacity + 1+AL->capacity/4;
	oldcontents = AL->contents;
	AL->contents = AL->Allocator->realloc(AL->contents,newcapacity*AL->ElementSize);
	if (AL->contents == NULL) {
		AL->RaiseError("iVector.Resize",CONTAINER_ERROR_NOMEMORY);
		AL->contents = oldcontents;
		return CONTAINER_ERROR_NOMEMORY;
	}
	AL->capacity = newcapacity;
	AL->timestamp++;
	return 1;
}

static int ResizeTo(Vector *AL,size_t newcapacity)
{
	void **oldcontents;
	
	if (AL == NULL) {
		return NullPtrError("ResizeTo");
	}
	oldcontents = AL->contents;
	AL->contents = AL->Allocator->realloc(AL->contents,newcapacity*AL->ElementSize);
	if (AL->contents == NULL) {
		AL->RaiseError("iVector.ResizeTo",CONTAINER_ERROR_NOMEMORY);
		AL->contents = oldcontents;
		return CONTAINER_ERROR_NOMEMORY;
	}
	AL->capacity = newcapacity;
	AL->timestamp++;
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
static int Add(Vector *AL,void *newval)
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
		int r = Resize(AL);
		if (r <= 0)
			return r;
	}
	p = AL->contents;
	p += AL->count*AL->ElementSize;
	memcpy(p,newval,AL->ElementSize);
	AL->timestamp++;
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
static int AddRange(Vector * AL,size_t n,void *data)
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
		newcontents = AL->Allocator->realloc(AL->contents,newcapacity*AL->ElementSize);
		if (newcontents == NULL) {
			AL->RaiseError("iVector.AddRange",CONTAINER_ERROR_NOMEMORY);
			return CONTAINER_ERROR_NOMEMORY;
		}
		AL->capacity = newcapacity;
		AL->contents = newcontents;
	}
	p = AL->contents;
	p += AL->count*AL->ElementSize;
	memcpy(p,data,n*AL->ElementSize);
	AL->count += n;
	AL->timestamp++;
	return 1;
}

static Vector *GetRange(Vector *AL, size_t start,size_t end)
{
	Vector *result;
	void *p;
	unsigned oldFlags;
	
	if (AL == NULL) {
		iError.RaiseError("iVector.GetRange",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	result = AL->VTable->Create(AL->ElementSize,AL->count);
	result->VTable = AL->VTable;
	if (AL->count == 0)
		return result;
	if (end >= AL->count)
		end = AL->count-1;
	if (start > end)
		return result;
	oldFlags = AL->Flags;
	AL->Flags = (unsigned)(~CONTAINER_READONLY);
	while (start <= end) {
		p = AL->VTable->GetElement(AL,start);
		result->VTable->Add(result,p);
		start++;
	}
	AL->Flags = result->Flags = oldFlags;
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
	if (AL->DestructorFn) {
		size_t i;
		unsigned char *p = AL->contents;
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

static int Contains(Vector *AL,void *data,void *ExtraArgs)
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
		ci.Container = AL;
		ci.ExtraArgs = ExtraArgs;
	}
	for (i = 0; i<AL->count;i++) {
		if (!AL->CompareFn(p,data,&ci))
			return 1;
		p += AL->ElementSize;
	}
	return 0;
}

static int Equal(Vector *AL1,Vector *AL2)
{
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
	if (memcmp(AL1->contents,AL2->contents,AL1->ElementSize*AL1->count) != 0)
		return 0;
	return 1;
}

static Vector *Copy(Vector *AL)
{
	Vector *result;
	size_t startsize,es;
	
	if (AL == NULL) {
		NullPtrError("Copy");
		return NULL;
	}

	result = AL->Allocator->malloc(sizeof(*result));
	if (result == NULL) {
		iError.RaiseError("iVector.Copy",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memset(result,0,sizeof(*result));
	startsize = AL->count;
	if (startsize <= 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * AL->ElementSize;
	result->contents = AL->Allocator->malloc(es);
	if (result->contents == NULL) {
		AL->RaiseError("iVector.Copy",CONTAINER_ERROR_NOMEMORY);
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
	return result;
}

static int CopyElement(Vector *AL,size_t idx, void *outbuf)
{
	char *p;
	if (AL == NULL || outbuf == NULL) {
		if (AL)
			AL->RaiseError("iVector.CopyElement",CONTAINER_ERROR_BADARG);
		else
			iError.RaiseError("iVector.CopyElement",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (idx >= AL->count) {
		AL->RaiseError("iVector.CopyElement",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	p = AL->contents;
	p += (idx)*AL->ElementSize;
	memcpy(outbuf,p,AL->ElementSize);
	return 1;
}

static void **CopyTo(Vector *AL)
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
		AL->RaiseError("iVector.CopyTo",CONTAINER_ERROR_NOMEMORY);
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

static int IndexOf(Vector *AL,void *data,void *ExtraArgs,size_t *result)
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
	ci.Container = AL;
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

static void *GetElement(Vector *AL,size_t idx)
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
		AL->RaiseError("iVector.GetElement",CONTAINER_ERROR_INDEX);
		return NULL;
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
		AL->RaiseError("iVector.InsertAt",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	if (AL->count >= (AL->capacity-1)) {
		int r = Resize(AL);
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
		AL->RaiseError("iVector.InsertIn",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	if (AL->ElementSize != newData->ElementSize) {
		AL->RaiseError("iVector.InsertIn",CONTAINER_ERROR_INCOMPATIBLE);
		return CONTAINER_ERROR_INCOMPATIBLE;
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
	return 1;
}

static int EraseAt(Vector *AL,size_t idx)
{
	char *p;

	if (AL == NULL) {
		return NullPtrError("EraseAt");
	}
	if (idx >= AL->count) {
		AL->RaiseError("iVector.Erase",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Erase");
	}
	p = AL->contents;
	if (AL->DestructorFn) {
		AL->DestructorFn(p+AL->ElementSize*idx);
	}
	if (idx < (AL->count-1)) {
		memmove(p+AL->ElementSize*idx,p+AL->ElementSize*(idx+1),(AL->count-idx)*AL->ElementSize);
	}
	AL->count--;
	AL->timestamp++;
	return 1;
}

static int Remove(Vector *AL,void *str)
{
	size_t idx;
	int i = IndexOf(AL,str,NULL,&idx);
	if (i < 0)
		return i;
	return EraseAt(AL,idx);
}

static int PushBack(Vector *AL,void *str)
{
	if (AL == NULL) {
		return NullPtrError("PushBack");
	}
	return InsertAt(AL,AL->count,str);
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
    AL->count--;
    if (result) {
        p = AL->contents;
        p += AL->ElementSize*AL->count;
        memcpy(result,p,AL->ElementSize);
    }
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
	int result = Clear(AL);
	if (result < 0)
		return result;
	AL->Allocator->free(AL);
	return result;
}

static ContainerMemoryManager *GetAllocator(Vector *AL)
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
		iError.RaiseError("iVector.Mismatch",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
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
	ci.Container = a1;
	ci.ExtraArgs = a2;
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
		AL->RaiseError("iVector.SetCapacity",CONTAINER_ERROR_NOMEMORY);
		return CONTAINER_ERROR_NOMEMORY;
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

	if (AL == NULL) {
			return NullPtrError("Apply");
	}
	if (Applyfn == NULL) {
		AL->RaiseError("iVector.Apply",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (AL->Flags&CONTAINER_READONLY) {
		pElem = AL->Allocator->malloc(AL->ElementSize);
		if (pElem == NULL) {
			AL->RaiseError("iVector.Apply",CONTAINER_ERROR_NOMEMORY);
			return CONTAINER_ERROR_NOMEMORY;
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
			iError.RaiseError("iVector.ReplaceAt",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (AL->Flags & CONTAINER_READONLY) {
		AL->RaiseError("iVector.ReplaceAt",CONTAINER_ERROR_READONLY);
		return CONTAINER_ERROR_READONLY;
	}
	if (idx >= AL->count) {
		AL->RaiseError("iVector.ReplaceAt",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	p = AL->contents;
	p += idx*AL->ElementSize;
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
		SC->RaiseError("iVector.IndexIn",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	if (iVector.GetElementSize(AL) != sizeof(size_t)) {
		SC->RaiseError("iVector.IndexIn",CONTAINER_ERROR_INCOMPATIBLE);
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
		iError.RaiseError("iVector.SetErrorFunction",CONTAINER_ERROR_BADARG);
		return 0;
	}
	old = AL->RaiseError;
	if (fn) AL->RaiseError = fn;
	return old;
}

static size_t Sizeof(Vector *AL)
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
	ci.Container = AL;
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
		AL1->RaiseError("iVector.Append",CONTAINER_ERROR_INCOMPATIBLE);
		return CONTAINER_ERROR_INCOMPATIBLE;
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
		AL->RaiseError("iVector.Reverse",CONTAINER_ERROR_NOMEMORY);
		return CONTAINER_ERROR_NOMEMORY;
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

struct VectorIterator {
	Iterator it;
	Vector *AL;
	size_t index;
	size_t timestamp;
	unsigned long Flags;
	void *Current;
	char ElementBuffer[1];
};

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
	if (ali->index >= AL->count)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		AL->RaiseError("GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	p = AL->contents;
	p += ali->index*AL->ElementSize;
	if (AL->Flags & CONTAINER_READONLY) {
		memcpy(ali->ElementBuffer,p,AL->ElementSize);
		p = ali->ElementBuffer;
	}
	ali->index++;
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

static Iterator *newIterator(Vector *AL)
{
	struct VectorIterator *result;

	if (AL == NULL) {
		NullPtrError("newIterator");
		return NULL;
	}
	result = AL->Allocator->malloc(sizeof(struct VectorIterator)+AL->ElementSize);
	if (result == NULL) {
		AL->RaiseError("iVector.newIterator",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetPrevious;
	result->it.GetFirst = GetFirst;
	result->it.GetCurrent = GetCurrent;
	result->it.GetLast = GetLast;
	result->AL = AL;
	result->Current = NULL;
	result->timestamp = AL->timestamp;
	return &result->it;
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

static size_t DefaultSaveFunction(const void *element,void *arg, FILE *Outfile)
{
	const unsigned char *str = element;
	size_t len = *(size_t *)arg;

	return fwrite(str,1,len,Outfile);
}

static size_t DefaultLoadFunction(void *element,void *arg, FILE *Infile)
{
	size_t len = *(size_t *)arg;

	return fread(element,1,len,Infile);
}

static int Save(Vector *AL,FILE *stream, SaveFunction saveFn,void *arg)
{
	size_t i;


	if (AL == NULL) {
		return NullPtrError("Save");
	}
	if (stream == NULL) {
		AL->RaiseError("iVector.Save",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (saveFn == NULL) {
		saveFn = DefaultSaveFunction;
		arg = &AL->ElementSize;
	}
	if (fwrite(&VectorGuid,sizeof(guid),1,stream) <= 0)
		return EOF;
	if (fwrite(AL,1,sizeof(Vector),stream) <= 0)
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
	if (fread(&Guid,sizeof(guid),1,stream) <= 0)
		return NULL;
	if (memcmp(&Guid,&VectorGuid,sizeof(guid))) {
		iError.RaiseError("iVector.Load",CONTAINER_ERROR_WRONGFILE);
		return NULL;
	}
	if (fread(&AL,1,sizeof(Vector),stream) <= 0)
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



static Vector *CreateWithAllocator(size_t elementsize,size_t startsize,ContainerMemoryManager *allocator)
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
	if (startsize <= 0)
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
		result->Allocator = allocator;
	}
	return result;
}

static Vector *Create(size_t elementsize,size_t startsize)
{
	return CreateWithAllocator(elementsize,startsize,CurrentMemoryManager);
}

static Vector *Init(Vector *result,size_t elementsize,size_t startsize)
{
	size_t es;
	
	memset(result,0,sizeof(*result));
	if (startsize <= 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * elementsize;
	result->contents = CurrentMemoryManager->malloc(es);
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
		result->Allocator = CurrentMemoryManager;
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


VectorInterface iVector = {
	GetCount,
	GetFlags, 
	SetFlags, 
	Clear,
	Contains,
	Remove, 
	Finalize,
	Apply,
	Equal,
	Copy,
	SetErrorFunction,
	Sizeof,
	newIterator,
	deleteIterator,
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
};
