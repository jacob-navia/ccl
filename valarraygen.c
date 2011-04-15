#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif

typedef struct tagSlice {
	size_t start;
	size_t length;
	size_t increment;
} SliceSpecs;
struct _ValArray {
    ValArrayInterface *VTable; /* The table of functions */
    size_t count;                  /* number of elements in the array */
    unsigned int Flags;            /* Read-only or other flags */
    size_t capacity;               /* allocated space in the contents vector */
    unsigned timestamp;            /* Incremented at each change */
    ErrorFunction RaiseError;      /* Error function */
    ContainerMemoryManager *Allocator;
    SliceSpecs *Slice;
    ElementType *contents;        /* The contents of the collection */
};

static ElementType GetElement(ValArray *AL,size_t idx);

static const guid ValArrayGuid = {0xba53f11e, 0x5879, 0x49e5,
{0x9e,0x3a,0xea,0x7d,0xd8,0xcb,0xd9,0xd6}
};
static size_t GetElementSize(const ValArray *AL);
static int DivisionByZero(const ValArray *AL,char *fnName)
{
	char buf[512];

	sprintf(buf,"iValArray.%s",fnName);
	AL->RaiseError(buf,CONTAINER_ERROR_DIVISION_BY_ZERO);
	return CONTAINER_ERROR_DIVISION_BY_ZERO;
}
static int ErrorReadOnly(ValArray *AL,char *fnName)
{
    char buf[512];

    sprintf(buf,"iValarray.%s",fnName);
    AL->RaiseError(buf,CONTAINER_ERROR_READONLY);
    return CONTAINER_ERROR_READONLY;
}

static int ErrorIncompatible(ValArray *AL, char *fnName)
{
    char buf[512];

    sprintf(buf,"iValarray.%s",fnName);
    AL->RaiseError(buf,CONTAINER_ERROR_INCOMPATIBLE);
    return CONTAINER_ERROR_INCOMPATIBLE;

}

static int NullPtrError(char *fnName)
{
	char buf[512];

	sprintf(buf,"iValArray.%s",fnName);
	iError.RaiseError(buf,CONTAINER_ERROR_BADARG);
	return CONTAINER_ERROR_BADARG;
}


static ValArray *Create(size_t startsize);

#define CHUNKSIZE 20
static int ValArrayDefaultCompareFn(const void *pleft,const void *pright,CompareInfo *notused)
{
	const ElementType *left = pleft,*right = pright;
	return *left < *right ? -1 : (*left > *right ? 1 : 0);
}

static size_t Size(const ValArray *AL)
{
	if (AL == NULL) {
		NullPtrError("Size");
		return 0;
	}
	return AL->count;
}
static unsigned GetFlags(const ValArray *AL)
{
	if (AL == NULL) {
		NullPtrError("GetFlags");
		return 0;
	}
	return AL->Flags;
}
static unsigned SetFlags(ValArray *AL,unsigned newval)
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


static int Resize(ValArray *AL)
{
	size_t newcapacity;
	ElementType *oldcontents;
	int r = 1;

	if (AL == NULL) {
		r = NullPtrError("Resize");
	}
	else {
		newcapacity = AL->capacity + 1+AL->capacity/4;
		oldcontents = AL->contents;
		AL->contents = AL->Allocator->realloc(AL->contents,newcapacity*sizeof(ElementType));
		if (AL->contents == NULL) {
			AL->RaiseError("iValArray.Resize",CONTAINER_ERROR_NOMEMORY);
			AL->contents = oldcontents;
			r = CONTAINER_ERROR_NOMEMORY;
		}
		else {
			AL->capacity = newcapacity;
			AL->timestamp++;
		}
	}
	return r;
}

static int ResizeTo(ValArray *AL,size_t newcapacity)
{
	ElementType *oldcontents;
	
	if (AL == NULL) {
		return NullPtrError("ResizeTo");
	}
	oldcontents = AL->contents;
	AL->contents = AL->Allocator->realloc(AL->contents,newcapacity*sizeof(ElementType));
	if (AL->contents == NULL) {
		AL->RaiseError("iValArray.ResizeTo",CONTAINER_ERROR_NOMEMORY);
		AL->contents = oldcontents;
		return CONTAINER_ERROR_NOMEMORY;
	}
	AL->capacity = newcapacity;
	AL->timestamp++;
	return 1;
}

/*------------------------------------------------------------------------
 Procedure:     Add ID:1
 Purpose:       Adds an item at the end of the ValArray
 Input:         The ValArray and the item to be added
 Output:        The number of items in the ValArray or negative if
                error
 Errors:
------------------------------------------------------------------------*/
static int Add(ValArray *AL,ElementType newval)
{
	int r;
	size_t pos;
	if (AL == NULL) {
		return NullPtrError("Add");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Add");
	}
	pos = AL->count;
	if (AL->Slice)
		pos += AL->Slice->increment-1;
	if ( pos >= AL->capacity) {
		if (pos != AL->count)
			r = ResizeTo(AL,pos+pos/2);
		else
			r = Resize(AL);
		if (r <= 0)
			return r;
	}
	AL->contents[pos] = newval;
	AL->timestamp++;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_ADD,&newval,AL->Slice);
	++AL->count;
	if (AL->Slice) {
		AL->Slice->length++;
		AL->count += AL->Slice->increment-1;
	}
	return 1;
}

/*------------------------------------------------------------------------
 Procedure:     AddRange ID:1
 Purpose:       Adds a range of data at the end of an existing
                arrraylist. 
 Input:         The ValArray and the data to be added
 Output:        One (OK) or negative error code
 Errors:        ValArray must be writable and data must be
                different of NULL
------------------------------------------------------------------------*/
static int AddRange(ValArray * AL,size_t n,ElementType *data)
{
	size_t i,newcapacity;
	size_t sliceIncrement=1;

	if (n == 0)
		return 1;
	if (AL == NULL) {
		return NullPtrError("AddRange");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"AddRange");
	}
	if (AL->Slice)
		sliceIncrement = AL->Slice->increment;
	newcapacity = AL->count+n*sliceIncrement;
	if (newcapacity >= AL->capacity-1) {
		ElementType *newcontents;
		newcapacity += AL->count/4;
		newcontents = AL->Allocator->realloc(AL->contents,newcapacity*sizeof(ElementType));
		if (newcontents == NULL) {
			AL->RaiseError("iValArray.AddRange",CONTAINER_ERROR_NOMEMORY);
			return CONTAINER_ERROR_NOMEMORY;
		}
		AL->capacity = newcapacity;
		AL->contents = newcontents;
	}
	if (sliceIncrement == 1) {
		memcpy(AL->contents+AL->count,data,n*sizeof(ElementType));
		AL->count += n;
	}
	else {
		size_t start = AL->count;
		memset(AL->contents+AL->count,0,sizeof(ElementType)*n*sliceIncrement);
		for (i=0; i<n;i++) {
			AL->contents[start] = *data++;
			start += sliceIncrement;
		}
	}
	AL->timestamp++;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_ADDRANGE,(void *)n,&data);

	return 1;
}

static ValArray *GetRange(ValArray *AL, size_t start,size_t end)
{
        ValArray *result=NULL;
        ElementType p;
        size_t top;

        if (AL->count == 0)
                return result;
        if (AL->Slice == NULL && end >= AL->count)
                end = AL->count-1;
	else if (AL->Slice && end >= AL->Slice->length)
		end = AL->Slice->length;
        if (start > end)
                return result;
        top = end - start;
        result = Create(top);
        if (result == NULL) {
                iError.RaiseError("iValArray.GetRange",CONTAINER_ERROR_NOMEMORY);
                return NULL;
        }
        while (start < end) {
                p = GetElement(AL,start);
                Add(result,p);
                start++;
        }
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
static int Clear(ValArray *AL)
{
	if (AL == NULL) {
		return NullPtrError("Clear");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Clear");
	}

	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_CLEAR,NULL,NULL);

	AL->count = 0;
	AL->timestamp = 0;
	AL->Flags = 0;
	if (AL->Slice) {
		AL->Allocator->free(AL->Slice);
		AL->Slice = NULL;
	}

	return 1;
}

static int Contains(ValArray *AL,ElementType data)
{
	size_t i,incr=1,start=0,top;
	ElementType *p;

	if (AL == NULL) {
		return NullPtrError("Contains");
	}
	p = AL->contents;
	top = AL->count;
	if (AL->Slice) {
		start = AL->Slice->start;
		top = AL->Slice->length;
		incr = AL->Slice->increment;
	}
	for (i = start; i<top;i++) {
		if (*p == data)
			return 1;
		p += incr;
	}
	return 0;
}

static int Equal(ValArray *AL1,ValArray *AL2)
{
	if (AL1 == AL2)
		return 1;
	if (AL1 == NULL || AL2 == NULL)
		return 0;
	if (AL1->count != AL2->count)
		return 0;
	if (AL1->Flags != AL2->Flags)
		return 0;
	if (AL1->count == 0)
		return 1;
	if (AL1->Slice == NULL && AL2->Slice)
		return 0;
	if (AL1->Slice && AL2->Slice == NULL)
		return 0;
	if (AL1->Slice && AL2->Slice) {
		if (AL1->Slice->start != AL2->Slice->start ||
		    AL1->Slice->length != AL2->Slice->length ||
		    AL1->Slice->increment != AL2->Slice->increment)
		return 0;
	} 
	if (AL1->Slice) {
		size_t i,start=AL1->Slice->start;

		for (i=0; i<AL1->Slice->length;i++) {
			if (AL1->contents[start] != AL2->contents[start])
				return 0;
			start += AL1->Slice->increment;
		}
	}
	else if (memcmp(AL1->contents,AL2->contents,sizeof(ElementType)*AL1->count) != 0)
		return 0;
	return 1;
}

static ValArray *Copy(ValArray *AL)
{
	ValArray *result;
	size_t startsize,es;
	
	if (AL == NULL) {
		NullPtrError("Copy");
		return NULL;
	}

	result = AL->Allocator->malloc(sizeof(*result));
	if (result == NULL) {
		iError.RaiseError("iValArray.Copy",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memset(result,0,sizeof(*result));
	startsize = AL->count;
	if (startsize <= 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * sizeof(ElementType);
	result->contents = AL->Allocator->malloc(es);
	if (result->contents == NULL) {
		AL->RaiseError("iValArray.Copy",CONTAINER_ERROR_NOMEMORY);
		AL->Allocator->free(result);
		return NULL;
	}
	else {
		memset(result->contents,0,es);
		result->capacity = startsize;
		result->VTable = &iValArrayInterface;
		result->RaiseError = iError.RaiseError;
		result->Allocator = AL->Allocator;
	}

	memcpy(result->contents,AL->contents,AL->count*sizeof(ElementType));
	result->Flags = AL->Flags;
	result->RaiseError = AL->RaiseError;
	result->VTable = AL->VTable;
	result->count = AL->count;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_COPY,result,NULL);
	return result;
}

static int CopyElement(ValArray *AL,size_t idx, ElementType *outbuf)
{
	if (AL == NULL ) {
		iError.RaiseError("iValArray.CopyElement",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (idx >= AL->count) {
		AL->RaiseError("iValArray.CopyElement",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	*outbuf = AL->contents[idx];
	return 1;
}

static ElementType *CopyTo(ValArray *AL)
{
	ElementType *result;
		
	if (AL == NULL) {
		NullPtrError("CopyTo");
		return NULL;
	}
	result = AL->Allocator->malloc((1+AL->count)*sizeof(ElementType));
	if (result == NULL) {
		AL->RaiseError("iValArray.CopyTo",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memcpy(result,AL->contents,AL->count * sizeof(ElementType));
	return result;
}

static int IndexOf(ValArray *AL,ElementType data,size_t *result)
{
	size_t i;
	ElementType *p;

	if (AL == NULL) {
		return NullPtrError("IndexOf");
	}
	p = AL->contents;
	for (i=0; i<AL->count;i++) {
		if (*p == data) {
			*result = i;
			return 1;
		}
		p++;
	}
	return CONTAINER_ERROR_NOTFOUND;
}

static ElementType GetElement(ValArray *AL,size_t idx)
{
	if (AL == NULL) {
		NullPtrError("GetElement");
		return 0;
	}
	if (idx >=AL->count ) {
		AL->RaiseError("iValArray.GetElement",CONTAINER_ERROR_INDEX);
		return (ElementType)0;
	}
	return AL->contents[idx];
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
static int InsertAt(ValArray *AL,size_t idx,ElementType newval)
{
	ElementType *p;
	
	if (AL == NULL) {
		return NullPtrError("InsertAt");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"InsertAt");
	}
	if (idx > AL->count) {
		AL->RaiseError("iValArray.InsertAt",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	if (AL->count >= (AL->capacity-1)) {
		int r = Resize(AL);
		if (r <= 0)
			return r;
	}
	p = AL->contents;
	if (idx < AL->count) {
		memmove(p+(idx+1),
				p+idx,
				(AL->count-idx+1)*sizeof(ElementType));
	}
	p[idx] = newval;
	AL->timestamp++;
	++AL->count;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_INSERT,&newval,(void *)idx);
	return 1;
}

static int Insert(ValArray *AL,ElementType newval)
{
	return InsertAt(AL,0,newval);
}

static int InsertIn(ValArray *AL, size_t idx, ValArray *newData)
{
	size_t newCount;
	ElementType *p;

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
		AL->RaiseError("iValArray.InsertIn",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	newCount = AL->count + newData->count;
	if (newCount >= (AL->capacity-1)) {
		int r = ResizeTo(AL,newCount);
		if (r <= 0)
			return r;
	}
	p = AL->contents;
	if (idx < AL->count) {
		memmove(p+(idx+newData->count),
				p+idx,
				(AL->count-idx)*sizeof(ElementType));
	}
	p += idx;
	memcpy(p,newData->contents,sizeof(ElementType)*newData->count);
	AL->timestamp++;
	AL->count = newCount;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_INSERT_IN,newData,NULL);
	return 1;
}

static int EraseAt(ValArray *AL,size_t idx)
{
	ElementType *p;

	if (AL == NULL) {
		return NullPtrError("EraseAt");
	}
	if (idx >= AL->count) {
		AL->RaiseError("iValArray.Erase",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Erase");
	}
	p = AL->contents+idx;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_ERASE,p,NULL);
	if (idx < (AL->count-1)) {
		memmove(p+idx,p+(idx+1),(AL->count-idx)*sizeof(ElementType));
	}
	AL->count--;
	AL->timestamp++;
	return 1;
}

static int Erase(ValArray *AL,ElementType data)
{
	size_t idx;
	int i = IndexOf(AL,data,&idx);
	if (i < 0)
		return i;
	return EraseAt(AL,idx);
}

static int PushBack(ValArray *AL,ElementType data)
{
	if (AL == NULL) {
		return NullPtrError("PushBack");
	}
	return InsertAt(AL,AL->count,data);
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
static int PopBack(ValArray *AL,ElementType *result)
{
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
       *result = AL->contents[AL->count-1];
    }
    AL->timestamp++;
    return 1;
}

/*------------------------------------------------------------------------
 Procedure:     Finalize ID:1
 Purpose:       Releases all memory associated with an ValArray,
                including the ValArray structure itself
 Input:         The ValArray to be destroyed
 Output:        The number of items that the ValArray contained or
                zero if error
 Errors:        Input must be writable
------------------------------------------------------------------------*/
static int Finalize(ValArray *AL)
{
	int result = Clear(AL);
	if (result < 0)
		return result;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_FINALIZE,NULL,NULL);
	AL->Allocator->free(AL);
	return result;
}

static ContainerMemoryManager *GetAllocator(ValArray *AL)
{
	if (AL == NULL) {
		return NULL;
	}
	return AL->Allocator;
}

static size_t GetCapacity(const ValArray *AL)
{
	if (AL == NULL) {
		NullPtrError("GetCapacity");
		return 0;
	}
	return AL->capacity;
}

static int Mismatch(ValArray *a1, ValArray *a2,size_t *mismatch)
{
	size_t siz,i;
	ElementType *p1,*p2;

	*mismatch = 0;
	if (a1 == a2)
		return 0;
	if (a1 == NULL || a2 == NULL || mismatch == NULL) {
		iError.RaiseError("iValArray.Mismatch",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	siz = a1->count;
	if (siz > a2->count)
		siz = a2->count;
	if (siz == 0)
		return 1;
	p1 = a1->contents;
	p2 = a2->contents;
	for (i=0;i<siz;i++) {
		if (*p1 != *p2)  {
			*mismatch = i;
			return 1;
		}
		p1++;
		p2++;
	}
	*mismatch = i;
	if (a1->count != a2->count)
		return 1;
	return 0;
}

static int SetCapacity(ValArray *AL,size_t newCapacity)
{
	ElementType *newContents;
	if (AL == NULL) {
		return NullPtrError("SetCapacity");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"SetCapacity");
	}
	newContents = AL->Allocator->malloc(newCapacity*sizeof(ElementType));
	if (newContents == NULL) {
		AL->RaiseError("iValArray.SetCapacity",CONTAINER_ERROR_NOMEMORY);
		return CONTAINER_ERROR_NOMEMORY;
	}
	memset(AL->contents,0,sizeof(ElementType)*newCapacity);
	AL->capacity = newCapacity;
	if (newCapacity > AL->count)
		newCapacity = AL->count;
	else if (newCapacity < AL->count)
		AL->count = newCapacity;
	if (newCapacity > 0) {
		memcpy(newContents,AL->contents,newCapacity*sizeof(ElementType));
	}
	AL->Allocator->free(AL->contents);
	AL->contents = newContents;
	AL->timestamp++;
	return 1;
}

static int Apply(ValArray *AL,int (*Applyfn)(ElementType,void *),void *arg)
{
	size_t i;
	ElementType *p;

	if (AL == NULL) {
			return NullPtrError("Apply");
	}
	if (Applyfn == NULL) {
		AL->RaiseError("iValArray.Apply",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	p=AL->contents;
	for (i=0; i<AL->count;i++) {
		Applyfn(p[i],arg);
	}
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
static int ReplaceAt(ValArray *AL,size_t idx,ElementType newval)
{
	if (AL == NULL) {
		iError.RaiseError("iValArray.ReplaceAt",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (AL->Flags & CONTAINER_READONLY) {
		AL->RaiseError("iValArray.ReplaceAt",CONTAINER_ERROR_READONLY);
		return CONTAINER_ERROR_READONLY;
	}
	if (idx >= AL->count) {
		AL->RaiseError("iValArray.ReplaceAt",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	if (AL->Flags & CONTAINER_HAS_OBSERVER) {
		iObserver.Notify(AL,CCL_REPLACEAT,&idx,&newval);
	}
	AL->contents[idx] = newval;
	AL->timestamp++;
	return 1;
}

static ValArray *IndexIn(ValArray *SC, ValArraySize_t *AL)
{
	ValArray *result = NULL;
	size_t i,top,idx;
	ElementType p;
	int r;

	if (SC == NULL ) {
		NullPtrError("IndexIn");
		return NULL;
	}
	if (AL == NULL) {
		SC->RaiseError("iValArray.IndexIn",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	top = iValArraySize_t.Size(AL);
	result = Create(top);
	for (i=0; i<top;i++) {
		idx = iValArraySize_t.GetElement(AL,i);
		p = GetElement(SC,idx);
		r = Add(result,p);
		if (r < 0) {
			Finalize(result);
			return NULL;
		}
	}
	return result;
}
static ErrorFunction SetErrorFunction(ValArray *AL,ErrorFunction fn)
{
	ErrorFunction old;

	if (AL == NULL) {
		iError.RaiseError("iValArray.SetErrorFunction",CONTAINER_ERROR_BADARG);
		return 0;
	}
	old = AL->RaiseError;
	if (fn) AL->RaiseError = fn;
	return old;
}

static size_t Sizeof(ValArray *AL)
{
	if (AL == NULL)
		return sizeof(ValArray);
	return sizeof(ValArray) + (AL->count * sizeof(ElementType));
}

/*------------------------------------------------------------------------
 Procedure:     SetCompareFunction ID:1
 Purpose:       Defines the function to be used in comparison of
                ValArray elements
 Input:         The ValArray and the new comparison function. If the new
                comparison function is NULL no changes are done.
 Output:        Returns the old function
 Errors:        None
------------------------------------------------------------------------*/
static CompareFunction SetCompareFunction(ValArray *l,CompareFunction fn)
{
	return NULL;
}

static int Sort(ValArray *AL)
{
	CompareInfo ci;

	if (AL == NULL) {
		return NullPtrError("Sort");
	}
	ci.Container = AL;
	ci.ExtraArgs = NULL;
	qsortEx(AL->contents,AL->count,sizeof(ElementType),ValArrayDefaultCompareFn,&ci);
	return 1;
}

/* Proposed by PWO 
*/
static int Append(ValArray *AL1, ValArray *AL2)
{
	size_t newCount;

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
	newCount = AL1->count + AL2->count;
	if (newCount >= (AL1->capacity-1)) {
		int r = ResizeTo(AL1,newCount);
		if (r <= 0)
			return r;
	}
	memcpy(AL1->contents+AL1->count,AL2->contents,sizeof(ElementType)*AL2->count);
	AL1->timestamp++;
	AL1->count = newCount;
	if (AL1->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL1,CCL_APPEND,AL2,NULL);
	AL2->Allocator->free(AL2);
    return 1;
}

/* Proposed by PWO 
*/
static int Reverse(ValArray *AL)
{
	size_t s;
	ElementType *p, *q;
	ElementType t;

	if (AL == NULL) {
		return NullPtrError("Reverse");
	}
	if (AL->Flags & CONTAINER_READONLY) {
		return ErrorReadOnly(AL,"Reverse");
	}
	if (AL->count < 2)
		return 1;

	p = AL->contents;
	s = AL->count;
	q = p + s*(AL->count-1);
	while ( p < q ) {
		t = *p;
		*p = *q;
		*q = t;
		p++;
		q--;
	}
	AL->timestamp++;
	return 1;
}


/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */

struct ValArrayIterator {
	Iterator it;
	ValArray *AL;
	size_t index;
	size_t timestamp;
	unsigned long Flags;
	ElementType *Current;
	ElementType ElementBuffer;
};

static void *GetNext(Iterator *it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	ValArray *AL;
	ElementType *p;

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
	p = AL->contents + ali->index;
	if (AL->Flags & CONTAINER_READONLY) {
		ali->ElementBuffer = *p;
		p = &ali->ElementBuffer;
	}
	ali->index++;
	ali->Current = p;
	return p;
}

static void *GetPrevious(Iterator *it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	ValArray *AL;
	ElementType *p;

	if (ali == NULL) {
		NullPtrError("GetPrevious");
		return NULL;
	}
	AL = ali->AL;
	if (ali->index >= AL->count || ali->index == 0)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		AL->RaiseError("iValArray.GetPrevious",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	p = AL->contents + ali->index;
	ali->index--;
	if (AL->Flags & CONTAINER_READONLY) {
		ali->ElementBuffer = *p;
		p = &ali->ElementBuffer;
	}
	ali->Current = p;
	return p;
}

static void *GetLast(Iterator *it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	ValArray *AL;
	ElementType *p;

	if (ali == NULL) {
		NullPtrError("GetLast");
		return NULL;
	}
	AL = ali->AL;
	if (AL->count == 0)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		AL->RaiseError("iValArray.GetLast",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	p = AL->contents + (AL->count-1);
	if (AL->Flags & CONTAINER_READONLY) {
		ali->ElementBuffer = AL->contents[AL->count-1];
		p = &ali->ElementBuffer;
	}
	ali->index = AL->count-1;
	ali->Current = p;
	return p;
}

static void *GetCurrent(Iterator *it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	if (ali == NULL) {
		NullPtrError("GetCurrent");
		return NULL;
	}
	return ali->Current;
}

static void *GetFirst(Iterator *it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;

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
		ali->ElementBuffer = ali->AL->contents[0];
		ali->Current = &ali->ElementBuffer;
	}
	return ali->Current;
}

static Iterator *newIterator(ValArray *AL)
{
	struct ValArrayIterator *result;

	if (AL == NULL) {
		NullPtrError("newIterator");
		return NULL;
	}
	result = AL->Allocator->malloc(sizeof(struct ValArrayIterator)+ sizeof(ElementType));
	if (result == NULL) {
		AL->RaiseError("iValArray.newIterator",CONTAINER_ERROR_NOMEMORY);
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
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	if (ali == NULL) {
		return NullPtrError("deleteIterator");
	}
	ali->AL->Allocator->free(it);
	return 1;
}

static int Save(ValArray *AL,FILE *stream)
{
	if (AL == NULL) {
		return NullPtrError("Save");
	}
	if (stream == NULL) {
		AL->RaiseError("iValArray.Save",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (fwrite(&ValArrayGuid,sizeof(guid),1,stream) <= 0)
		return EOF;
	if (fwrite(AL,sizeof(ValArray),1,stream) <= 0)
		return EOF;
	if (fwrite(AL->contents,sizeof(ElementType),AL->count,stream) != AL->count)
		return EOF;
	return 1;
}

#if 0
static int SaveToBuffer(ValArray *AL,char *stream, size_t *bufferlen,SaveFunction saveFn,void *arg)
{
	size_t i,len;
	int elemLen;
	
	if (AL == NULL) {
		return NullPtrError("iValArray.SaveToBuffer");
	}
	if (bufferlen == NULL) {
		AL->RaiseError("iValArray.SaveToBuffer",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (stream)
		len = *bufferlen;
	else {
		len = sizeof(guid) + sizeof(ValArray);
	}
	if (saveFn == NULL) {
		saveFn = DefaultSaveFunction;
		arg = &AL->ElementSize;
	}
	*bufferlen = 0;
	if (stream) {
		if (len >= sizeof(guid)) {
			memcpy(stream,&ValArrayGuid,sizeof(guid));
			len -= sizeof(guid);
			stream += sizeof(guid);
		}
		else return EOF;
		if (len >= sizeof(ValArray)) {
			memcpy(stream,AL,sizeof(ValArray));
			len -= sizeof(ValArray);
			stream += sizeof(ValArray);
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

static ValArray *Load(FILE *stream)
{
	ElementType *p;
	ValArray *result,AL;
	guid Guid;

	if (stream == NULL) {
		NullPtrError("Load");
		return NULL;
	}
	if (fread(&Guid,sizeof(guid),1,stream) <= 0)
		return NULL;
	if (memcmp(&Guid,&ValArrayGuid,sizeof(guid))) {
		iError.RaiseError("iValArray.Load",CONTAINER_ERROR_WRONGFILE);
		return NULL;
	}
	if (fread(&AL,1,sizeof(ValArray),stream) <= 0)
		return NULL;
	result = Create(AL.count);
	if (result) {
		p = result->contents;
		if (fread(p,sizeof(ElementType),AL.count,stream) != AL.count)
			return NULL;
		result->Flags = AL.Flags;
		result->count = AL.count;
	}
	else iError.RaiseError("iValArray.Load",CONTAINER_ERROR_NOMEMORY);
	return result;
}



static ValArray *CreateWithAllocator(size_t startsize,ContainerMemoryManager *allocator)
{
	ValArray *result;
	size_t es;
	
	/* Allocate space for the array list header structure */
	result = allocator->malloc(sizeof(*result));
	if (result == NULL) {
		iError.RaiseError("iValArray.Create",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memset(result,0,sizeof(*result));
	if (startsize <= 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * sizeof(ElementType);
	result->contents = allocator->malloc(es);
	if (result->contents == NULL) {
		iError.RaiseError("iValArray.Create",CONTAINER_ERROR_NOMEMORY);
		allocator->free(result);
		result = NULL;
	}
	else {
		memset(result->contents,0,es);
		result->capacity = startsize;
		result->VTable = &iValArrayInterface;
		result->RaiseError = iError.RaiseError;
		result->Allocator = allocator;
	}
	return result;
}

static ValArray *Create(size_t startsize)
{
	return CreateWithAllocator(startsize,CurrentMemoryManager);
}

static ValArray *Init(ValArray *result,size_t startsize)
{
	size_t es;
	
	memset(result,0,sizeof(*result));
	if (startsize <= 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * sizeof(ElementType);
	result->contents = CurrentMemoryManager->malloc(es);
	if (result->contents == NULL) {
		iError.RaiseError("iValArray.Init",CONTAINER_ERROR_NOMEMORY);
		result = NULL;
	}
	else {
		memset(result->contents,0,es);
		result->capacity = startsize;
		result->VTable = &iValArrayInterface;
		result->RaiseError = iError.RaiseError;
		result->Allocator = CurrentMemoryManager;
	}
	return result;
}

static size_t GetElementSize(const ValArray *AL)
{
	return sizeof(ElementType);
}
static DestructorFunction SetDestructor(ValArray *cb,DestructorFunction fn)
{
	return NULL;
}

static int SumTo(ValArray *left,ValArray *right)
{
	size_t i;

	if (left->count != right->count) {
		return ErrorIncompatible(left,"SumTo");
	}
	for (i=0; i<left->count; i++) 
		left->contents[i] += right->contents[i];
	return 1;
}

static int SumToScalar(ValArray *left,ElementType right)
{
        size_t i;

        for (i=0; i<left->count; i++)
                left->contents[i] += right;
        return 1;
}


static int SubtractFrom(ValArray *left,ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"SubtractFrom");
        }
        for (i=0; i<left->count; i++)
                left->contents[i] -= right->contents[i];
        return 1;
}

static int SubtractFromScalar(ValArray *left,ElementType right)
{
        size_t i;

        for (i=0; i<left->count; i++)
                left->contents[i] -= right;
        return 1;
}


static int MultiplyWith(ValArray *left,ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"MultiplyWith");
        }
        for (i=0; i<left->count; i++)
                left->contents[i] *= right->contents[i];
        return 1;
}

static int MultiplyWithScalar(ValArray *left,ElementType right)
{
        size_t i;

        for (i=0; i<left->count; i++)
                left->contents[i] *= right;
        return 1;
}


static int DivideBy(ValArray *left,ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"DivideBy");
        }
        for (i=0; i<left->count; i++)
		if (right->contents[i])
                	left->contents[i] /= right->contents[i];
		else
			DivisionByZero(left,"DivideBy");
        return 1;
}

static int DivideByScalar(ValArray *left,ElementType right)
{
        size_t i;

	if (right == 0)
		return DivisionByZero(left,"DivideByScalar");
        for (i=0; i<left->count; i++)
                left->contents[i] /= right;
        return 1;
}


static unsigned char *CompareEqual(ValArray *left,ValArray *right,unsigned char *bytearray)
{
	size_t len = left->count,i,siz;

	if (len != right->count) {
		ErrorIncompatible(left,"Compare");
		return NULL;
	}
	siz = 1 + len/CHAR_BIT;
	if (bytearray == NULL)
		bytearray = left->Allocator->malloc(siz);
	if (bytearray == NULL) {
		iError.RaiseError("ValArray.Compare",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memset(bytearray,0,siz);
	for (i=0; i<len;i++) {
		bytearray[i/CHAR_BIT] |= (left->contents[i] == right->contents[i]);
		if ((CHAR_BIT-1) != (i&(CHAR_BIT-1)))
			bytearray[i] <<= 1;
	}
	return bytearray;
}

static unsigned char *CompareEqualScalar(ValArray *left,ElementType right,unsigned char *bytearray)
{
        size_t len = left->count,i,siz;

        siz = 1 + len/CHAR_BIT;
        if (bytearray == NULL)
                bytearray = left->Allocator->malloc(siz);
        if (bytearray == NULL) {
                iError.RaiseError("ValArray.Compare",CONTAINER_ERROR_NOMEMORY);
                return NULL;
        }
        memset(bytearray,0,siz);
        for (i=0; i<len;i++) {
                bytearray[i/CHAR_BIT] |= (left->contents[i] == right);
                if ((CHAR_BIT-1) != (i&(CHAR_BIT-1)))
                        bytearray[i] <<= 1;
        }
        return bytearray;
}

static char *Compare(ValArray *left,ValArray *right, char *bytearray)
{
        size_t len = left->count,i,siz;

        if (len != right->count) {
                ErrorIncompatible(left,"Compare");
                return NULL;
        }
        siz =  len;
        if (bytearray == NULL)
                bytearray = left->Allocator->malloc(siz);
        if (bytearray == NULL) {
                iError.RaiseError("ValArray.Compare",CONTAINER_ERROR_NOMEMORY);
                return NULL;
        }
        memset(bytearray,0,siz);
        for (i=0; i<len;i++) {
                bytearray[i] = (left->contents[i] < right->contents[i]) ?
			-1 : (left->contents[i] > right->contents[i]) ? 1 : 0;
        }
        return bytearray;
}

static char *CompareScalar(ValArray *left,ElementType right,char *bytearray)
{
        size_t len = left->count,i,siz;

        siz =  len;
        if (bytearray == NULL)
                bytearray = left->Allocator->malloc(siz);
        if (bytearray == NULL) {
                iError.RaiseError("ValArray.Compare",CONTAINER_ERROR_NOMEMORY);
                return NULL;
        }
        memset(bytearray,0,siz);
        for (i=0; i<len;i++) {
                bytearray[i] = (left->contents[i] < right) ?
                        -1 : (left->contents[i] > right) ? 1 : 0;
        }
        return bytearray;
}


static int Fill(ValArray *dst,ElementType data)
{
	size_t top = dst->count;
	size_t i;
	for (i=0; i<top;i++) {
		dst->contents[i]=data;
	}
	return 1;
}

#ifdef IS_UNSIGNED
static int Or(ValArray *left,ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"Or");
        }
        for (i=0; i<left->count; i++)
                left->contents[i] |= right->contents[i];
        return 1;
}
static int OrScalar(ValArray *left,ElementType right)
{
        size_t i;

        for (i=0; i<left->count; i++)
                left->contents[i] |= right;
        return 1;
}

static int And(ValArray *left,ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"And");
        }
        for (i=0; i<left->count; i++)
                left->contents[i] &= right->contents[i];
        return 1;
}
static int AndScalar(ValArray *left,ElementType right)
{
        size_t i;

        for (i=0; i<left->count; i++)
                left->contents[i] &= right;
        return 1;
}

static int Xor(ValArray *left,ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"Xor");
        }
        for (i=0; i<left->count; i++)
                left->contents[i] ^= right->contents[i];
        return 1;
}

static int XorScalar(ValArray *left,ElementType right)
{
        size_t i;

        for (i=0; i<left->count; i++)
                left->contents[i] ^= right;
        return 1;
}

static int Not(ValArray *left)
{
        size_t i;

        for (i=0; i<left->count; i++)
                left->contents[i] = ~left->contents[i];
        return 1;
}

static int RightShift(ValArray *,int);

static int LeftShift(ValArray *data,int shift)
{
        size_t i;

	if (shift < 0)
		return RightShift(data,-shift);
        for (i=0; i<data->count; i++)
                data->contents[i] <<= shift;
        return 1;
}

static int RightShift(ValArray *data,int shift)
{
        size_t i;

        if (shift < 0)
                return LeftShift(data,-shift);
	else if (shift == 0)
		return 1;
        for (i=0; i<data->count; i++)
                data->contents[i] >>= shift;
        return 1;
}


#endif

#ifdef __IS_SIZE_T
static ValArraySize_t *BuildSlice(size_t start,size_t size,size_t increment)
{
	size_t i;
	ValAraySize_t result = iValArraySize_t.Create(size);

	if (result) {
		for (i=0; i<size;i++) {
			result->contents[i] = start;
			start += increment;
		}
	}
	return result;
}
#endif

ValArrayInterface iValArray = {
	Size,
	GetFlags, 
	SetFlags, 
	Clear,
	Contains,
	Erase, 
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
/* ValArray specific fields */
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
	SumTo,
	SubtractFrom,
	MultiplyWith,
	DivideBy,
	SumToScalar,
	SubtractFromScalar,
	MultiplyWithScalar,
	DivideByScalar,
	CompareEqual,
	CompareEqualScalar,
	Compare,
	CompareScalar,
	Fill,
#ifdef IS_UNSIGNED
	Or,
	And,
	Xor,
	Not,
	LeftShift,
	RightShift,
	OrScalar,
	AndScalar,
	XorScalar,
#endif
};
