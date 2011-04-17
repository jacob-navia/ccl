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

static ElementType GetElement(const ValArray *AL,size_t idx);

static size_t GetElementSize(const ValArray *AL);
static int DivisionByZero(const ValArray *AL,char *fnName)
{
	char buf[512];

	sprintf(buf,"iValArray.%s",fnName);
	AL->RaiseError(buf,CONTAINER_ERROR_DIVISION_BY_ZERO);
	return CONTAINER_ERROR_DIVISION_BY_ZERO;
}

static int ErrorIncompatible(const ValArray *AL, char *fnName)
{
    char buf[512];

    sprintf(buf,"iValarray.%s",fnName);
    AL->RaiseError(buf,CONTAINER_ERROR_INCOMPATIBLE);
    return CONTAINER_ERROR_INCOMPATIBLE;

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
	return AL->count;
}
static unsigned GetFlags(const ValArray *AL)
{
	return AL->Flags;
}
static unsigned SetFlags(ValArray *AL,unsigned newval)
{
	int oldval;
	oldval = AL->Flags;
	AL->Flags = newval;
	return oldval;
}


static int Resize(ValArray *AL)
{
	size_t newcapacity;
	ElementType *oldcontents;
	int r = 1;

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
	return r;
}

static int ResizeTo(ValArray *AL,size_t newcapacity)
{
	ElementType *oldcontents;
	
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
static int AddRange(ValArray * AL,size_t n,const ElementType *data)
{
	size_t i,newcapacity;
	size_t sliceIncrement=1;

	if (n == 0)
		return 1;
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
		iObserver.Notify(AL,CCL_ADDRANGE,(void *)n,(ElementType *)&data);

	return 1;
}

static ValArray *GetRange(const ValArray *AL, size_t start,size_t end)
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
	
	result = AL->Allocator->malloc(sizeof(*result));
	if (result == NULL) {
		iError.RaiseError("iValArray.Copy",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memset(result,0,sizeof(*result));
	startsize = (AL->Slice == NULL) ? AL->count : AL->Slice->length;
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
	if (AL->Slice == NULL)
		memcpy(result->contents,AL->contents,AL->count*sizeof(ElementType));
	else {
		size_t idx = AL->Slice->start,i;
		for (i=0; i<AL->Slice->length; i++) {
			result->contents[i] = AL->contents[idx];
			idx += AL->Slice->increment;
		}
	}
	result->Flags = AL->Flags;
	result->RaiseError = AL->RaiseError;
	result->VTable = AL->VTable;
	result->count = (AL->Slice == NULL) ? AL->count : AL->Slice->length;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_COPY,result,NULL);
	return result;
}

static int CopyElement(ValArray *AL,size_t idx, ElementType *outbuf)
{
	size_t top = (AL->Slice) ? AL->Slice->length : AL->count;
	if (idx >= top) {
		AL->RaiseError("iValArray.CopyElement",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	if (AL->Slice) {
		idx *= AL->Slice->start + idx * AL->Slice->increment;

	}
	*outbuf = AL->contents[idx];
	return 1;
}

static ElementType *CopyTo(ValArray *AL)
{
	ElementType *result;
	size_t top = (AL->Slice) ? AL->Slice->length : AL->count;
	result = AL->Allocator->calloc(1+top,sizeof(ElementType));
	if (result == NULL) {
		AL->RaiseError("iValArray.CopyTo",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	if (AL->Slice) {
                size_t idx = AL->Slice->start,i;
		for (i=0; i<top; i++) {
			result[i] = AL->contents[idx];
			idx += AL->Slice->increment;
		} 
	}
	else memcpy(result,AL->contents,top * sizeof(ElementType));
	return result;
}

static int IndexOf(ValArray *AL,ElementType data,size_t *result)
{
	size_t i,start=0,incr=1,top=AL->count;
	ElementType *p;

	if (AL->Slice) {
		start = AL->Slice->start;
		incr = AL->Slice->increment;
		top = AL->Slice->length;
	}
	p = &AL->contents[start];
	for (i=0; i<top;i++) {
		if (*p == data) {
			*result = i;
			return 1;
		}
		p += incr;
	}
	return CONTAINER_ERROR_NOTFOUND;
}

static ElementType GetElement(const ValArray *AL,size_t idx)
{
	size_t start=0,incr=1,top=AL->count;

	if (AL->Slice) {
		start = AL->Slice->start;
		incr = AL->Slice->increment;
		top = AL->Slice->length;
	}
	if (idx >=top ) {
		AL->RaiseError("iValArray.GetElement",CONTAINER_ERROR_INDEX);
		return (ElementType)0;
	}
	idx = start+idx*incr;
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
	size_t top=AL->count,incr = 1,start=0;

	if (AL->Slice) {
		top = AL->Slice->length;
		incr = AL->Slice->increment;
		start = AL->Slice->start;
	}
	if (idx >= top) {
		AL->RaiseError("iValArray.Erase",CONTAINER_ERROR_INDEX);
		return CONTAINER_ERROR_INDEX;
	}
	p = AL->contents + start + idx * AL->Slice->increment;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_ERASE,p,NULL);
	if (idx < (AL->count-1)) {
		memmove(p+idx,p+(idx+1),(AL->count-idx)*sizeof(ElementType));
	}
	AL->count--;
	if (AL->Slice && start >= AL->count) {
		AL->Allocator->free(AL->Slice);
		AL->Slice = NULL;
	}
	if (AL->Slice && idx > start+ top * incr) {
		AL->Slice->length = 1+(AL->count-start)/AL->Slice->increment;
	}
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
    size_t idx;
    if (AL->count == 0)
            return 0;
    AL->count--;
    if (AL->Slice) {
        idx = AL->Slice->start + (AL->Slice->length-1)*AL->Slice->increment;
        AL->Slice->length--;
        if (AL->Slice->length == 0) {
            AL->Allocator->free(AL->Slice);
            AL->Slice = NULL;
        }
    }
    else idx = AL->count;
    if (result) {
       *result = AL->contents[idx];
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
	return AL->capacity;
}

static int Mismatch(ValArray *a1, ValArray *a2,size_t *mismatch)
{
	size_t siz1=a1->count,i,siz2=a2->count,incr1=1,incr2=1,start1=0,start2=0;
	ElementType *p1,*p2;

	*mismatch = 0;
	if (a1 == a2)
		return 0;
	if (a1->Slice) {
		siz1 = a1->Slice->length;
		start1 = a1->Slice->start;
		incr1 = a1->Slice->increment;
	}
	if (a2->Slice) {
		siz2 = a2->Slice->length;
		start2 = a2->Slice->start;
		incr2 = a2->Slice->increment;
	}
	if (siz1 > siz2)
		siz1 = siz2;
	if (siz1 == 0)
		return 1;
	p1 = a1->contents+start1;
	p2 = a2->contents+start2;
	for (i=start1;i<siz1;i++) {
		if (*p1 != *p2)  {
			*mismatch = i*incr1;
			return 1;
		}
		p1 += incr1;
		p2 += incr2;
	}
	*mismatch = i*incr1;
	if (a1->count != a2->count)
		return 1;
	return 0;
}

static int SetCapacity(ValArray *AL,size_t newCapacity)
{
	ElementType *newContents;
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
	size_t i,start=0,incr=1,top=AL->count;
	ElementType *p;

	if (AL->Slice) {
		top = AL->Slice->length;
		incr = AL->Slice->increment;
		start = AL->Slice->start;
	}
	p=AL->contents;
	for (i=start; i<top;i += incr) {
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

static ValArray *IndexIn(const ValArray *SC, const ValArraySize_t *AL)
{
	ValArray *result = NULL;
	size_t i,top,idx;
	ElementType p;
	int r;

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

	AL = ali->AL;
	if (ali->index >= AL->count)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		AL->RaiseError("GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	p = AL->contents + ali->index;
	ali->index++;
	ali->Current = p;
	return p;
}

static void *GetPrevious(Iterator *it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	ValArray *AL;
	ElementType *p;

	AL = ali->AL;
	if (ali->index >= AL->count || ali->index == 0)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		AL->RaiseError("iValArray.GetPrevious",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	p = AL->contents + ali->index;
	ali->index--;
	ali->Current = p;
	return p;
}

static void *GetLast(Iterator *it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	ValArray *AL;
	ElementType *p;

	AL = ali->AL;
	if (AL->count == 0)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		AL->RaiseError("iValArray.GetLast",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	p = AL->contents + (AL->count-1);
	ali->index = AL->count-1;
	ali->Current = p;
	return p;
}

static void *GetCurrent(Iterator *it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	return ali->Current;
}

static void *GetFirst(Iterator *it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;

	if (ali->AL->count == 0) {
		ali->Current = NULL;
		return NULL;
	}
	ali->index = 1;
	ali->Current = ali->AL->contents;
	return ali->Current;
}

static Iterator *newIterator(ValArray *AL)
{
	struct ValArrayIterator *result;

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
	ali->AL->Allocator->free(it);
	return 1;
}

static int Save(ValArray *AL,FILE *stream)
{
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

static int SumTo(ValArray *left,const ValArray *right)
{
	size_t i,top_left=left->count,start_left=0,incr_left=1,idx_left;
	size_t top_right = right->count,start_right=0,incr_right=1,idx_right;

	if (left->Slice) {
		start_left = left->Slice->start;
		incr_left = left->Slice->increment;
		top_left = left->Slice->length;
	}
	if (right->Slice)  {
		top_right = right->Slice->length;
		start_right = right->Slice->start;
		incr_right = right->Slice->increment;
	}
	if (top_right != top_left) {
		return ErrorIncompatible(left,"SumTo");
	}
	idx_left = start_left;
	idx_right = start_right;
	for (i=0; i<top_left; i++) {
		left->contents[idx_left] += right->contents[idx_right];
		idx_left += incr_left;
		idx_right += incr_right;
	}
	return 1;
}

static int SumToScalar(ValArray *left,ElementType right)
{
	size_t i,top_left=left->count,start_left=0,incr_left=1,idx_left;

	if (left->Slice) {
		start_left = left->Slice->start;
		incr_left = left->Slice->increment;
		top_left = left->Slice->length;
	}
	idx_left = start_left;
        for (i=start_left; i<top_left; i++) {
                left->contents[idx_left] += right;
		idx_left += incr_left;
	}
        return 1;
}


static int SubtractFrom(ValArray *left,const ValArray *right)
{
	size_t i,top_left=left->count,start_left=0,incr_left=1,idx_left;
	size_t top_right = right->count,start_right=0,incr_right=1,idx_right;

	if (left->Slice) {
		start_left = left->Slice->start;
		incr_left = left->Slice->increment;
		top_left = left->Slice->length;
	}
	if (right->Slice)  {
		top_right = right->Slice->length;
		start_right = right->Slice->start;
		incr_right = right->Slice->increment;
	}
	if (top_right != top_left) {
		return ErrorIncompatible(left,"SumTo");
	}
	idx_left = start_left;
	idx_right = start_right;
	for (i=0; i<top_left; i++) {
		left->contents[idx_left] -= right->contents[idx_right];
		idx_left += incr_left;
		idx_right += incr_right;
	}
	return 1;
}
static int SubtractFromScalar(ValArray *left,ElementType right)
{
	size_t i,top_left=left->count,start_left=0,incr_left=1,idx_left;

	if (left->Slice) {
		start_left = left->Slice->start;
		incr_left = left->Slice->increment;
		top_left = left->Slice->length;
	}
	idx_left = start_left;
        for (i=start_left; i<top_left; i++) {
                left->contents[idx_left] -= right;
		idx_left += incr_left;
	}
        return 1;
}

static int MultiplyWith(ValArray *left,const ValArray *right)
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


static int DivideBy(ValArray *left,const ValArray *right)
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

#ifdef __IS_INTEGER__
static int Mod(ValArray *left,const ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"DivideBy");
        }
        for (i=0; i<left->count; i++)
                if (right->contents[i])
                        left->contents[i] %= right->contents[i];
                else
                        DivisionByZero(left,"Mod");
        return 1;
}

static int ModScalar(ValArray *left, const ElementType right)
{
        size_t i;

        if (right == 0)
                return DivisionByZero(left,"ModScalar");
        for (i=0; i<left->count; i++)
                left->contents[i] %= right;
        return 1;
}
#endif

static unsigned char *CompareEqual(const ValArray *left,const ValArray *right,unsigned char *bytearray)
{
	size_t len = left->count,i,siz;

	if (len != right->count) {
		ErrorIncompatible(left,"CompareEqual");
		return NULL;
	}
	siz = 1 + len/CHAR_BIT;
	if (bytearray == NULL)
		bytearray = left->Allocator->malloc(siz);
	if (bytearray == NULL) {
		iError.RaiseError("ValArray.CompareEqual",CONTAINER_ERROR_NOMEMORY);
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

static unsigned char *CompareEqualScalar(const ValArray *left, const ElementType right,unsigned char *bytearray)
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

static char *Compare(const ValArray *left,const ValArray *right, char *bytearray)
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

static char *CompareScalar(const ValArray *left,const ElementType right,char *bytearray)
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


static int FillSequential(ValArray *dst,ElementType start,ElementType increment)
{
	size_t i;
	
	if (dst->Slice) {
		size_t s = dst->Slice->start;
		size_t l = dst->Slice->length;
		size_t inc = dst->Slice->increment;
		for (i=s; i<l;i += inc) {
			dst->contents[i] = start;
			start += increment;
		}
	}
	else for (i=0; i<dst->count;i++) {
		dst->contents[i] = start;
		start += increment;
	}
	return 1;
}
static int Fill(ValArray *dst,ElementType data)
{
	return FillSequential(dst,data,0);
}

#ifdef __IS_UNSIGNED__
static int Or(ValArray *left, const ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"Or");
        }
        for (i=0; i<left->count; i++)
                left->contents[i] |= right->contents[i];
        return 1;
}
static int OrScalar(ValArray *left, const ElementType right)
{
        size_t i;

        for (i=0; i<left->count; i++)
                left->contents[i] |= right;
        return 1;
}

static int And(ValArray *left, const ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"And");
        }
        for (i=0; i<left->count; i++)
                left->contents[i] &= right->contents[i];
        return 1;
}
static int AndScalar(ValArray *left,const ElementType right)
{
        size_t i;

        for (i=0; i<left->count; i++)
                left->contents[i] &= right;
        return 1;
}

static int Xor(ValArray *left,const ValArray *right)
{
        size_t i;

        if (left->count != right->count) {
                return ErrorIncompatible(left,"Xor");
        }
        for (i=0; i<left->count; i++)
                left->contents[i] ^= right->contents[i];
        return 1;
}

static int XorScalar(ValArray *left, const ElementType right)
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

static int SetSlice(ValArray *array,size_t start,size_t length,size_t increment)
{
	int result = 0;
	if (start >= array->count ||
	   increment == 0 || length == 0 ) {
		iError.RaiseError("SetSlice",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if ((array->count-start)/(length*increment) > array->count) {
		length = 1+(array->count-start)/increment;
	}

	if (array->Slice == NULL) {
		array->Slice = array->Allocator->malloc(sizeof(SliceSpecs));
		if (array->Slice == NULL) {
			iError.RaiseError("SetSlice",CONTAINER_ERROR_NOMEMORY);
			return CONTAINER_ERROR_NOMEMORY;
		}
		result++;
	}
	array->Slice->start = start;
	array->Slice->length = length;
	array->Slice->increment = increment;
	
	return result;
}

static int GetSlice(ValArray *array,size_t *start,size_t *length, size_t *incr)
{
	if (array->Slice == NULL) {
		if (start) *start = 0;
		if (length) *length = 0;
		if (incr) *incr = 0;
		return 0;
	}
	if (start) *start = array->Slice->start;
	if (length) *length = array->Slice->length;
	if (incr) *incr = array->Slice->increment;
	return 1;
}

static int ResetSlice(ValArray *array)
{
	if (array->Slice == NULL)
		return 0;
	array->Allocator->free(array->Slice);
	array->Slice = NULL;
	return 1;
}

static ElementType Max(const ValArray *src)
{
	ElementType result=MinElementType;
	size_t start=0,length=src->count,incr=1,i;

	if (src == NULL || src->count == 0)
		return MaxElementType;
	if (src->Slice) {
		start = src->Slice->start;
		incr = src->Slice->increment;
		length = src->Slice->length;
	}
	result = src->contents[start];
	for (i=start; i<length;i += incr) {
		if (result < src->contents[i])
			result = src->contents[i];
	}
	return result;
}
static ElementType Min(const ValArray *src)
{
        ElementType result=MaxElementType;
        size_t start=0,length=src->count,incr=1,i;

        if (src == NULL || src->count == 0)
                return MaxElementType;
	if (src->Slice) {
		start = src->Slice->start;
		incr = src->Slice->increment;
		length = src->Slice->length;
	}
	result = src->contents[start];
        for (i=start; i<length;i += incr) {
                if (result > src->contents[i])
                        result = src->contents[i];
        }
        return result;
}


ValArrayInterface iValArrayInterface = {
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
	FillSequential,
	SetSlice,
	ResetSlice,
	GetSlice,
	Max,
	Min,
#ifdef __IS_UNSIGNED__
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
#ifdef __IS_INTEGER__
	Mod,
	ModScalar,
#endif
};
