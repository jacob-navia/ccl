#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif
#include <math.h>
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
    ContainerAllocator *Allocator;
    SliceSpecs *Slice;
    ElementType *contents;        /* The contents of the collection */
};

static ElementType GetElement(const ValArray *AL,size_t idx);
static int Finalize(ValArray *AL);

static size_t GetElementSize(const ValArray *AL);
static int doerror(char *fnName,int code)
{
	char buf[512];
	
	(void)snprintf(buf,sizeof(buf),"iValArray.%s",fnName);
	iValArrayInterface.RaiseError(buf,code);
	return code;
}

static int DivisionByZero(char *fnName)
{
	return doerror(fnName,CONTAINER_ERROR_DIVISION_BY_ZERO);
}

static int ErrorIncompatible(char *fnName)
{
    return doerror(fnName,CONTAINER_ERROR_INCOMPATIBLE);
}

static int NoMemory(char *fnName)
{
    return doerror(fnName,CONTAINER_ERROR_NOMEMORY);
}

static int IndexError(char *fnName)
{
    return doerror(fnName,CONTAINER_ERROR_INDEX);
}


static ValArray *Create(size_t startsize);

#define CHUNKSIZE 20
static int ValArrayDefaultCompareFn(const void *pleft,const void *pright)
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
	unsigned oldval = AL->Flags;
	AL->Flags = newval;
	return oldval;
}


static int ResizeTo(ValArray *AL,size_t newcapacity)
{
	ElementType *oldcontents;
	
	if (AL->capacity == newcapacity) return 0;
	oldcontents = AL->contents;
	AL->contents = AL->Allocator->realloc(AL->contents,newcapacity*sizeof(ElementType));
	if (AL->contents == NULL) {
		AL->contents = oldcontents;
		return NoMemory("ResizeTo");
	}
	AL->capacity = newcapacity;
	AL->timestamp++;
	return 1;
}

static int grow(ValArray *AL)
{
	return ResizeTo(AL,AL->capacity+1+AL->capacity/4);
}

static int Resize(ValArray *AL, size_t newSize)
{
	ElementType *p;

	if (AL->count < newSize) return ResizeTo(AL,newSize);
	p = AL->Allocator->realloc(AL->contents,newSize*sizeof(ElementType));
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
 Purpose:       Adds an item at the end of the ValArray
 Input:         The ValArray and the item to be added
 Output:        The number of items in the ValArray or negative if
 error
 Errors:
 ------------------------------------------------------------------------*/
static int Add(ValArray *AL,ElementType newval)
{
	int r;
	size_t pos = AL->count;
	if (AL->Slice)
		pos += AL->Slice->increment-1;
	if (pos >= AL->capacity) {
		if (pos != AL->count)
			r = ResizeTo(AL,pos+pos/2);
		else
			r = grow(AL);
		if (r <= 0)
			return r;
	}
	AL->contents[pos] = newval;
	AL->timestamp++;
	++AL->count;
	if (AL->Slice) {
		AL->Slice->length++;
		AL->count += AL->Slice->increment-1;
	}
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_ADD,&newval,AL->Slice);
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
			return NoMemory("AddRange");
		}
		AL->capacity = newcapacity;
		AL->contents = newcontents;
	}
	if (sliceIncrement == 1) {
		if (data)
			memcpy(AL->contents+AL->count,data,n*sizeof(ElementType));
		else
			memset(AL->contents+AL->count,0,n*sizeof(ElementType));
		AL->count += n;
	}
	else {
		size_t start = AL->count;
		memset(AL->contents+AL->count,0,sizeof(ElementType)*n*sliceIncrement);
		for (i=0; i<n;i++) {
			if (data)
				AL->contents[start] = *data++;
			else
				AL->contents[start] = 0;
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
	int r;
	
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
		NoMemory("GetRange");
		return NULL;
	}
	while (start < end) {
		p = GetElement(AL,start);
		r=Add(result,p);
		if (r < 0) {
			Finalize(result);
			return NULL;
		}
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

static int Contains(const ValArray *AL,ElementType data)
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

static int Equal(const ValArray *AL1, const ValArray *AL2)
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

static ValArray *Copy(const ValArray *AL)
{
	ValArray *result;
	size_t startsize,es;
	
	result = AL->Allocator->malloc(sizeof(*result));
	if (result == NULL) {
		NoMemory("Copy");
		return NULL;
	}
	memset(result,0,sizeof(*result));
	startsize = (AL->Slice == NULL) ? AL->count : AL->Slice->length;
	if (startsize == 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * sizeof(ElementType);
	result->contents = AL->Allocator->malloc(es);
	if (result->contents == NULL) {
		NoMemory("Copy");
		AL->Allocator->free(result);
		return NULL;
	}
	else {
		memset(result->contents,0,es);
		result->capacity = startsize;
		result->VTable = &iValArrayInterface;
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
	result->VTable = AL->VTable;
	result->count = (AL->Slice == NULL) ? AL->count : AL->Slice->length;
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_COPY,result,NULL);
	return result;
}

static int CopyElement(const ValArray *AL,size_t idx, ElementType *outbuf)
{
	size_t top = (AL->Slice) ? AL->Slice->length : AL->count;
	if (idx >= top) {
		return IndexError("CopyElement");
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
		NoMemory("CopyTo");
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
		IndexError("GetElement");
		return MinElementType;
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
		return IndexError(".InsertAt");
	}
	if (AL->count >= (AL->capacity-1)) {
		int r = grow(AL);
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
		return IndexError("InsertIn");
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
		p = AL->contents + start + idx * AL->Slice->increment;
		idx += start + idx*incr;
	}
	p = AL->contents +idx;
	if (idx >= top) {
		return IndexError("Erase");
	}
	if (AL->Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_ERASE_AT,(void *)idx,NULL);
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

static int RemoveRange(ValArray *AL,size_t start, size_t end)
{
        if (AL->count == 0)
                return 0;
	if (AL->Slice) {
		size_t sliceEnd = AL->Slice->start + AL->Slice->length;
		start += AL->Slice->start;
		if (start >= sliceEnd)
			return 0;
		end += AL->Slice->start;
		if (end > sliceEnd)
			end = sliceEnd;
	}
        if (end > AL->count)
                end = AL->count;
        if (start == end) return 0;
        if (start >= AL->count) {
                return 0;
        }
        if (end < AL->count)
        memmove(AL->contents+start,
                AL->contents+end,
                (AL->count-end)*sizeof(ElementType));
        AL->count -= end - start;
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
	unsigned Flags = AL->Flags;
	int result = Clear(AL);
	if (result < 0)
		return result;
	if (Flags & CONTAINER_HAS_OBSERVER)
		iObserver.Notify(AL,CCL_FINALIZE,NULL,NULL);
	AL->Allocator->free(AL->contents);
	AL->Allocator->free(AL);
	return result;
}

static ContainerAllocator *GetAllocator(const ValArray *AL)
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

static int Mismatch(const ValArray *a1, const ValArray *a2,size_t *mismatch)
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
		return NoMemory("SetCapacity");
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

static int ForEach(ValArray *AL, ElementType (*ApplyFn)(ElementType))
{
	size_t i,start=0,incr=1,top=AL->count;
	ElementType *p;
	
	if (AL->Slice) {
		top = AL->Slice->length;
		incr = AL->Slice->increment;
		start = AL->Slice->start;
	}
	p = AL->contents;
	for (i=start; i<top;i += incr) {
		p[i] = ApplyFn(p[i]);
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
		return IndexError("ReplaceAt");
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
	ValArray *al = (ValArray *)AL;
	size_t i,top,idx,*contents= (size_t *)al->contents;
	ElementType p;
	
	top = Size(al);
	result = Create(top);
	if (result == NULL)
		return NULL;
	for (i=0; i<top;i++) {
		idx = contents[i];
		p = GetElement(SC,idx);
		result->contents[result->count++]=p;
	}
	return result;
}
static ErrorFunction SetErrorFunction(ValArray *AL,ErrorFunction fn)
{
	ErrorFunction old;
	
	if (AL == NULL) return iError.RaiseError;
	old = iValArrayInterface.RaiseError;
	if (fn) iValArrayInterface.RaiseError = fn;
	return old;
}

static size_t Sizeof(const ValArray *AL)
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
	if (AL->Slice) {
		size_t i,j=0;
		ElementType *sliceTab = AL->Allocator->calloc(sizeof(ElementType),AL->Slice->length);
		if (sliceTab == NULL) return NoMemory("Sort");
		for (i=AL->Slice->start;i<AL->Slice->length;i += AL->Slice->increment) {
			sliceTab[j++] = AL->contents[i];
		}
		qsort(sliceTab,AL->Slice->length,sizeof(ElementType),ValArrayDefaultCompareFn);
		j=0;
		for (i=AL->Slice->start; i<AL->Slice->length; i += AL->Slice->increment) {
			AL->contents[i] = sliceTab[j++];
		}
		AL->Allocator->free(sliceTab);
	}
	else qsort(AL->contents,AL->count,sizeof(ElementType),ValArrayDefaultCompareFn);
	return 1;
}

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

static int Reverse(ValArray *AL)
{
	size_t s;
	ElementType *p, *q;
	ElementType t;
	
	if (AL->count < 2)
		return 1;
	
	if (AL->Slice) {
		p = AL->contents+AL->Slice->start;
		s = AL->Slice->length;
		q = p + (s*AL->Slice->increment-1);
		while (p<q) {
			t = *p;
			*p = *q;
			*q = t;
			p += AL->Slice->increment;
			q -= AL->Slice->increment;
		}
	}
	else {
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
	}
	AL->timestamp++;
	return 1;
}

/* http://eli.thegreenplace.net/2008/08/29/space-efficient-list-rotation */
static int RotateLeft(ValArray *AL, size_t n)
{
	ElementType *p,*q,t;
	
	n %= AL->count;
	if (n == 0)
		return 1;
	/* Reverse the first partition */
	if (n > 1) {
		p = AL->contents;
		q = AL->contents+n-1;
		while (p < q) {
			t =*p;
			*p = *q;
			*q = t;
			p++;
			q--;
		}
	}
	/* Reverse the second partition */
	p = AL->contents+n;
	q = AL->contents+AL->count-1;
	if (n < AL->count-1) {
		while (p<q) {
			t = *p;
			*p = *q;
			*q = t;
			p++,q--;
		}
	}
	/* Now reverse the whole */
	p = AL->contents;
	q = p+(AL->count-1);
	while (p<q) {
		t = *p;
		*p = *q;
		*q = t;
		p++,q--;
	}
	return 1;
}


static int RotateRight(ValArray *AL, size_t n)
{
	ElementType *p,*q,t;
	
	n %= AL->count;
	if (n == 0)
		return 1;
	/* Reverse the first partition */
	p = AL->contents+AL->count-n;
	q = AL->contents+AL->count-1;
	while (p < q) {
		t =*p;
		*p = *q;
		*q = t;
		p++;
		q--;
	}
	/* Reverse the second partition */
	p = AL->contents;
	q = AL->contents+AL->count-n-1;
	if (n < AL->count-1) {
		while (p<q) {
			t = *p;
			*p = *q;
			*q = t;
			p++,q--;
		}
	}
	/* Now reverse the whole */
	p = AL->contents;
	q = p+(AL->count-1);
	while (p<q) {
		t = *p;
		*p = *q;
		*q = t;
		p++,q--;
	}
	return 1;
}


/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */

struct ValArrayIterator {
	Iterator it;
	ValArray *AL;
	size_t index;
	unsigned timestamp;
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
		iValArrayInterface.RaiseError("GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	ali->index++;
	p = AL->contents + ali->index;
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
		iValArrayInterface.RaiseError("iValArray.GetPrevious",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	ali->index--;
	p = AL->contents + ali->index;
	ali->Current = p;
	return p;
}
static void *Seek(Iterator *it,size_t idx)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	ValArray *AL;
	ElementType *p;
	
	AL = ali->AL;
	if (idx >= AL->count)
		return NULL;
	if (ali->timestamp != AL->timestamp) {
		iError.RaiseError("iValArray.Seek",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	ali->index = idx;
	p = AL->contents + ali->index;
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
		iValArrayInterface.RaiseError("iValArray.GetLast",CONTAINER_ERROR_OBJECT_CHANGED);
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
	ali->index = 0;
	ali->Current = ali->AL->contents;
	return ali->Current;
}

static int ReplaceWithIterator(Iterator *it, void *pdata,int direction) 
{
    struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	ElementType *data = (ElementType *)pdata;
	int result;
	size_t pos;
	
	if (ali->AL->count == 0)
		return 0;
    if (ali->timestamp != ali->AL->timestamp) {
        iError.RaiseError("Replace",CONTAINER_ERROR_OBJECT_CHANGED);
        return CONTAINER_ERROR_OBJECT_CHANGED;
    }
	if (ali->AL->Flags & CONTAINER_READONLY) {
		iError.RaiseError("Replace",CONTAINER_ERROR_READONLY);
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
		result = ReplaceAt(ali->AL,pos,*data);
	}
	if (result >= 0) {
		ali->timestamp = ali->AL->timestamp;
	}
	return result;
}


static Iterator *NewIterator(ValArray *AL)
{
	struct ValArrayIterator *result;
	
	result = AL->Allocator->calloc(1,sizeof(struct ValArrayIterator)+sizeof(ElementType));
	if (result == NULL) {
		NoMemory("NewIterator");
		return NULL;
	}
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetPrevious;
	result->it.GetFirst = GetFirst;
	result->it.GetCurrent = GetCurrent;
	result->it.GetLast = GetLast;
	result->it.Replace = ReplaceWithIterator;
	result->it.Seek = Seek;
	result->AL = AL;
	result->Current = NULL;
	result->timestamp = AL->timestamp;
	result->index = -1;
	return &result->it;
}

static int InitIterator(ValArray *AL,void *buf)
{
        struct ValArrayIterator *result = buf;

        result->it.GetNext = GetNext;
        result->it.GetPrevious = GetPrevious;
        result->it.GetFirst = GetFirst;
        result->it.GetCurrent = GetCurrent;
        result->it.GetLast = GetLast;
        result->it.Replace = ReplaceWithIterator;
        result->it.Seek = Seek;
        result->AL = AL;
        result->Current = NULL;
        result->timestamp = AL->timestamp;
        result->index = -1;
        return 1;
}


static int deleteIterator(Iterator * it)
{
	struct ValArrayIterator *ali = (struct ValArrayIterator *)it;
	ali->AL->Allocator->free(it);
	return 1;
}

static int Save(const ValArray *AL,FILE *stream)
{
	if (fwrite(&ValArrayGuid,sizeof(guid),1,stream) == 0)
		return EOF;
	if (fwrite(AL,sizeof(ValArray),1,stream) == 0)
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
	
	if (fread(&Guid,sizeof(guid),1,stream) == 0)
		return NULL;
	if (memcmp(&Guid,&ValArrayGuid,sizeof(guid))) {
		iError.RaiseError("iValArray.Load",CONTAINER_ERROR_WRONGFILE);
		return NULL;
	}
	if (fread(&AL,1,sizeof(ValArray),stream) == 0)
		return NULL;
	result = Create(AL.count);
	if (result) {
		p = result->contents;
		if (fread(p,sizeof(ElementType),AL.count,stream) != AL.count)
			return NULL;
		result->Flags = AL.Flags;
		result->count = AL.count;
	}
	else NoMemory("Load");
	return result;
}



static ValArray *CreateWithAllocator(size_t startsize,ContainerAllocator *allocator)
{
	ValArray *result;
	size_t es;
	
	/* Allocate space for the array list header structure */
	result = allocator->malloc(sizeof(*result));
	if (result == NULL) {
		NoMemory("Create");
		return NULL;
	}
	memset(result,0,sizeof(*result));
	if (startsize == 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * sizeof(ElementType);
	result->contents = allocator->malloc(es);
	if (result->contents == NULL) {
		NoMemory("Create");
		allocator->free(result);
		result = NULL;
	}
	else {
		memset(result->contents,0,es);
		result->capacity = startsize;
		result->VTable = &iValArrayInterface;
		result->Allocator = allocator;
	}
	return result;
}

static ValArray *Create(size_t startsize)
{
	return CreateWithAllocator(startsize,CurrentAllocator);
}

static ValArray *InitializeWith(size_t n,ElementType *data)
{
	ValArray *result = Create(n);
	if (result == NULL)
		return result;
	memcpy(result->contents,data,n*sizeof(ElementType));
	result->count = n;
	return result;
}


static ValArray *Init(ValArray *result,size_t startsize)
{
	size_t es;
	
	memset(result,0,sizeof(*result));
	if (startsize == 0)
		startsize = DEFAULT_START_SIZE;
	es = startsize * sizeof(ElementType);
	result->contents = CurrentAllocator->malloc(es);
	if (result->contents == NULL) {
		NoMemory("Init");
		result = NULL;
	}
	else {
		memset(result->contents,0,es);
		result->capacity = startsize;
		result->VTable = &iValArrayInterface;
		result->Allocator = CurrentAllocator;
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
		return ErrorIncompatible("SumTo");
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
		return ErrorIncompatible("SumTo");
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
static int SubtractScalarFrom(ValArray *left,ElementType right)
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


static int SubtractFromScalar(ElementType left,ValArray *right)
{
	size_t i,top_right=right->count,start_right=0,incr_right=1,idx_right;
	
	if (right->Slice) {
		start_right = right->Slice->start;
		incr_right = right->Slice->increment;
		top_right = right->Slice->length;
	}
	idx_right = start_right;
	for (i=start_right; i<top_right; i++) {
		right->contents[idx_right] = left -right->contents[idx_right];
		idx_right += incr_right;
	}
	return 1;
}

static int MultiplyWith(ValArray *left,const ValArray *right)
{
	size_t i;
	
	if (left->count != right->count) {
		return ErrorIncompatible("MultiplyWith");
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
		return ErrorIncompatible("DivideBy");
	}
	for (i=0; i<left->count; i++)
		if (right->contents[i] != 0)
			left->contents[i] /= right->contents[i];
		else
			DivisionByZero("DivideBy");
	return 1;
}

static int DivideByScalar(ValArray *left,ElementType right)
{
	size_t i;
	
	if (right == 0)
		return DivisionByZero("DivideByScalar");
	for (i=0; i<left->count; i++)
		left->contents[i] /= right;
	return 1;
}

static int DivideScalarBy(ValArray *right, ElementType left)
{
	size_t i;
	
	if (left == 0) {
		memset(right->contents,0,sizeof(ElementType)*right->count);
		return 1;
	}
	for (i=0; i<right->count; i++)
		right->contents[i] = left / right->contents[i];
	return 1;
}


#ifdef __IS_INTEGER__
static int Mod(ValArray *left,const ValArray *right)
{
	size_t i;
	
	if (left->count != right->count) {
		return ErrorIncompatible("DivideBy");
	}
	for (i=0; i<left->count; i++)
		if (right->contents[i])
			left->contents[i] %= right->contents[i];
		else
			DivisionByZero("Mod");
	return 1;
}

static int ModScalar(ValArray *left, const ElementType right)
{
	size_t i;
	
	if (right == 0)
		return DivisionByZero("ModScalar");
	for (i=0; i<left->count; i++)
		left->contents[i] %= right;
	return 1;
}
#endif

static Mask *CompareEqual(const ValArray *left,const ValArray *right,Mask *bytearray)
{
	
	size_t left_len = left->count,left_incr = 1,left_start=0;
	size_t right_len = right->count,right_incr=1,right_start = 0;
	size_t siz,i,j;
	
	if (left->Slice) {
		left_start = left->Slice->start;
		left_incr = left->Slice->increment;
		left_len = left->Slice->length;
	}
	if (right->Slice) {
		right_start = right->Slice->start;
		right_incr = right->Slice->increment;
		right_len = right->Slice->length;
	}
	if (left_len != right_len) {
		ErrorIncompatible("Compare");
		return NULL;
	}
	siz = left_len;
	if (bytearray == NULL || bytearray->length < left_len) {
		if (bytearray) iMask.Finalize(bytearray);
		bytearray = iMask.Create(left_len);
		if (bytearray == NULL) {
			NoMemory("Compare");
			return NULL;
		}
	}
	else memset(bytearray->data,0,siz);
	j=right_start;
	for (i=left_start; i<left_len;i += left_incr) {
		bytearray->data[i] = (left->contents[i] == right->contents[j]);
		j += right_incr;
	}
	bytearray->length = left_len;
	return bytearray;
}

static Mask *CompareEqualScalar(const ValArray *left, const ElementType right,Mask *bytearray)
{
	size_t len = left->count,i,siz;
	
	siz = 1 + len/CHAR_BIT;
	if (bytearray == NULL)
		bytearray = left->Allocator->malloc(siz);
	if (bytearray == NULL) {
		NoMemory("Compare");
		return NULL;
	}
	memset(bytearray,0,siz);
	for (i=0; i<len;i++) {
		bytearray->data[i/CHAR_BIT] |= (left->contents[i] == right);
		if ((CHAR_BIT-1) != (i&(CHAR_BIT-1)))
			bytearray->data[i] <<= 1;
	}
	return bytearray;
}

static char *Compare(const ValArray *left,const ValArray *right, char *bytearray)
{
	size_t left_len = left->count,left_incr = 1,left_start=0;
	size_t right_len = right->count,right_incr=1,right_start = 0;
	size_t siz,i,j,k;
	
	if (left->Slice) {
		left_start = left->Slice->start;
		left_incr = left->Slice->increment;
		left_len = left->Slice->length;
	}
	if (right->Slice) {
		right_start = right->Slice->start;
		right_incr = right->Slice->increment;
		right_len = right->Slice->length;
	}
	if (left_len != right_len) {
		ErrorIncompatible("Compare");
		return NULL;
	}
	siz = left_len * sizeof(ElementType);
	if (bytearray == NULL)
		bytearray = left->Allocator->malloc(left_len);
	if (bytearray == NULL) {
		NoMemory("Compare");
		return NULL;
	}
	memset(bytearray,0,siz);
	j = right_start;
	i = left_start;
	for (k=0;k<left_len;k++) {
		bytearray[k] = (left->contents[i] < right->contents[j]) ?
		-1 : (left->contents[i] > right->contents[j]) ? 1 : 0;
		j += right_incr;
		i += left_incr;
	}
	return bytearray;
}

#ifndef __IS_INTEGER__
/* Adapted from <http://www-personal.umich.edu/~streak/>
 Knuth, D. E. (1998). The Art of Computer Programming.
 Volume 2: Seminumerical Algorithms. 3rd ed. Addison-Wesley.
 Section 4.2.2, p. 233. ISBN 0-201-89684-2.
 
 Input parameters:
 x1, x2: numbers to be compared
 epsilon: determines tolerance
 */
static Mask *FCompare(const ValArray *left,const ValArray *right, Mask *bytearray,ElementType tolerance)
{
	size_t left_len = left->count,left_incr = 1,left_start=0;
	size_t right_len = right->count,right_incr=1,right_start = 0;
	size_t i,j,k;
	ElementType delta,difference,x1,x2;
	
	if (left->Slice) {
		left_start = left->Slice->start;
		left_incr = left->Slice->increment;
		left_len = left->Slice->length;
	}
	if (right->Slice) {
		right_start = right->Slice->start;
		right_incr = right->Slice->increment;
		right_len = right->Slice->length;
	}
	if (left_len != right_len) {
		ErrorIncompatible("Compare");
		return NULL;
	}
	if (bytearray == NULL) {
		NoMemory("Compare");
		return NULL;
	}
	
	if (bytearray == NULL) {
		bytearray= CurrentAllocator->malloc(left_len+sizeof(Mask));
		if (bytearray == NULL) {
			iError.RaiseError("ValArray.Fcompare",CONTAINER_ERROR_NOMEMORY);
			return NULL;
		}
		bytearray->Allocator = CurrentAllocator;
		bytearray->length = left_len;
		
	}
	memset(bytearray->data,0,left_len);
	j = right_start;
	i = left_start;
	for (k=0;k<left_len;k++) {
		int exponent;
		x1 = left->contents[i];
		x2 = right->contents[j];
		frexp(fabs(x1) > fabs(x2) ? x1 : x2,&exponent);
		delta = (ElementType)ldexp(tolerance,exponent);
		difference = x1-x2;
		if (difference > delta)
			bytearray->data[k] = 1; /* x1 > x2 */
		else if (difference < -delta)
			bytearray->data[k] = -1; /* x1 < x2 */
		else	bytearray->data[k] = 0;
		j += right_incr;
		i += left_incr;
	}
	return bytearray;
}


static int Inverse(ValArray *s)
{
	size_t start=0,length=s->count,incr=1,i;
	if (s == NULL || s->count == 0)
		return 0;
	if (s->Slice) {
		start = s->Slice->start;
		incr = s->Slice->increment;
		length = s->Slice->length;
	}
	for (i=start; i<length; i += incr) {
		if (s->contents[i]==0) {
			return DivisionByZero("Inverse");
		}
		s->contents[i] =1/s->contents[i];
	}
	return 1;
}
#endif


static char *CompareScalar(const ValArray *left,const ElementType right,char *bytearray)
{
	size_t left_len = left->count,left_incr = 1,left_start=0;
	size_t i,j=0;
	
	
	if (left->Slice) {
		left_start = left->Slice->start;
		left_incr = left->Slice->increment;
		left_len = left->Slice->length;
	}
	if (bytearray == NULL)
		bytearray = left->Allocator->malloc(left_len);
	if (bytearray == NULL) {
		NoMemory("Compare");
		return NULL;
	}
	
	memset(bytearray,0,left_len);
	for (i=left_start; i<left_len;i += left_incr) {
		bytearray[j++] = (left->contents[i] < right) ?
		-1 : (left->contents[i] > right) ? 1 : 0;
	}
	return bytearray;
}

static ValArray *CreateSequence(size_t n,ElementType start, ElementType increment)
{
	ValArray *result = Create(n);
	size_t i;
	
	if (result == NULL) {
		iValArrayInterface.RaiseError("iValArray.Create",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	for (i=0; i<n;i++) {
		result->contents[i] = start;
		start += increment;
	}
	result->count = n;
	return result;
}

static int FillSequential(ValArray *dst,size_t length,ElementType start,ElementType increment)
{
	size_t i,top = length;
	int r;
	
	if (top > dst->capacity) {
		r = ResizeTo(dst,top);
		if (r < 0)
			return r;
	}
	if (dst->Slice) {
		size_t s = dst->Slice->start;
		size_t l = dst->Slice->length;
		size_t inc = dst->Slice->increment;
		if (l > length) l = length;
		for (i=s; i<l;i += inc) {
			dst->contents[i] = start;
			start += increment;
		}
	}
	else for (i=0; i<top;i++) {
		dst->contents[i] = start;
		start += increment;
	}
	return 1;
}
static int Memset(ValArray *dst,ElementType data,size_t length)
{
	return FillSequential(dst,length,data,0);
}

#ifdef __IS_UNSIGNED__
static int Or(ValArray *left, const ValArray *right)
{
	size_t i;
	
	if (left->count != right->count) {
		return ErrorIncompatible("Or");
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
		return ErrorIncompatible("And");
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
		return ErrorIncompatible("Xor");
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
	size_t i,s=0,top=left->count,incr=1;
	
	if (left->Slice) {
		s = left->Slice->start;
		top = left->Slice->length;
		incr = left->Slice->increment;
	}
	for (i=s; i<top;i += incr) {
		left->contents[i] = ~left->contents[i];
		s += incr;
	}
	return 1;
}

static int RightShift(ValArray *,int);

static int LeftShift(ValArray *data,int shift)
{
	size_t i,s=0,top=data->count,incr=1;
	if (shift < 0)
		return RightShift(data,-shift);
	else if (shift == 0)
		return 1;
	if (data->Slice) {
		s = data->Slice->start;
		top = data->Slice->length;
		incr = data->Slice->increment;
	}
	for (i=s; i<top; i += incr)
		data->contents[i] <<= shift;
	return 1;
}

static int RightShift(ValArray *data,int shift)
{
	size_t i,s=0,incr=1,top=data->count;
	
	if (shift < 0)
		return LeftShift(data,-shift);
	else if (shift == 0)
		return 1;
	
	if (data->Slice) {
		s = data->Slice->start;
		top = data->Slice->length;
		incr = data->Slice->increment;
	}
	for (i=s; i<top; i += incr)
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
			return NoMemory("SetSlice");
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

#ifndef __IS_UNSIGNED__
static int Abs(ValArray *src)
{
	size_t start=0,length=src->count,incr=1,i;
	
	if (src->count == 0)
		return 0;
	if (src->Slice) {
		start = src->Slice->start;
		incr = src->Slice->increment;
		length = src->Slice->length;
	}
	for (i=start; i<length;i += incr) {
		if (0 > src->contents[i])
			src->contents[i] = -src->contents[i];
	}
	return 1;
}
#endif

static ElementType Accumulate(const ValArray *src)
{
	size_t start=0,length=src->count,incr=1,i;
	ElementType result = 0;
	
	if (src->count == 0)
		return 0;
	if (src->Slice) {
		start = src->Slice->start;
		incr = src->Slice->increment;
		length = src->Slice->length;
	}
	for (i=start; i<length;i += incr) {
		result += src->contents[i];
	}
	return result;
}


static ElementType Product(const ValArray *src)
{
	size_t start=0,length=src->count,incr=1,i;
	ElementType result = 1;
	
	if (src->count == 0)
		return 0;
	if (src->Slice) {
		start = src->Slice->start;
		incr = src->Slice->increment;
		length = src->Slice->length;
	}
	for (i=start; i<length;i += incr) {
		result *= src->contents[i];
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

static void *RaiseError(const char *msg,int code,...)
{
	return iError.RaiseError(msg,code);
}

static int Fprintf(const ValArray *src,FILE *out,const char *fmt)
{
	size_t start=0,length=src->count,incr=1,i;
	int result=0,r;
	if (src->Slice) {
		start = src->Slice->start;
		incr = src->Slice->increment;
		length = src->Slice->length;
	}
	for (i=start; i<length; i += incr) {
		r = fprintf(out,fmt,src->contents[i]);
		if (r <= 0) {
			result = r;
			break;
		}
		result += r;
	}
	if (result > 0)
		result +=fprintf(out,"\n");
	return result;
}

static int Select(ValArray *src,const Mask *m)
{
	size_t i,offset=0;
	if (m->length != src->count) {
		iError.RaiseError("Select",CONTAINER_ERROR_BADMASK,src,m);
                return CONTAINER_ERROR_BADMASK;
	}
	for (i=0; i<m->length;i++) {
		if (m->data[i]) {
			if (i != offset)
				src->contents[offset] = src->contents[i];
			offset++;
		}
	}
	if (offset < i) {
		memset(src->contents+offset,0,sizeof(ElementType)*(i-offset));
	}
	src->count = offset;
	return 1;
}

static ValArray  *SelectCopy(const ValArray *src,const Mask *m)
{
	size_t i,offset=0;
	ValArray *result;
	
	if (m->length != src->count) {
		iError.RaiseError("SelectCopy",CONTAINER_ERROR_BADMASK,src,m);
		return NULL;
	}
	result = Create(src->count);
	if (result == NULL) {
		NoMemory("SelectCopy");
		return NULL;
	}
	for (i=0; i<m->length;i++) {
		if (m->data[i]) {
			result->contents[offset] = src->contents[i];
			offset++;
		}
	}
	result->count = offset;
	return result;
}


static ElementType *GetData(const ValArray *cb)
{
	if (cb == NULL) {
		iError.RaiseError("GetData",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	if (cb->Flags&CONTAINER_READONLY) {
		iError.RaiseError("GetData",CONTAINER_ERROR_READONLY);
		return NULL;
	}
	return cb->contents;
}

static ElementType Back(const ValArray *cb)
{
	ElementType r = 0;
	if (cb->Flags&CONTAINER_READONLY) {
		iError.RaiseError("Back",CONTAINER_ERROR_READONLY);
	}
	else if (cb->count > 0)
		r = cb->contents[cb->count-1];
	return r;
}

static ElementType Front(const ValArray *cb)
{
	ElementType r = 0;
	if (cb->Flags&CONTAINER_READONLY) {
		iError.RaiseError("Front",CONTAINER_ERROR_READONLY);
	}
	else if (cb->count > 0)
		r = cb->contents[0];
	return r;
}

static size_t SizeofIterator(const ValArray *cb)
{
	return sizeof (struct ValArrayIterator) + sizeof(ElementType);
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
	
	RaiseError,
	SumTo,
	SubtractFrom,
	MultiplyWith,
	DivideBy,
	SumToScalar,
	SubtractScalarFrom,
	SubtractFromScalar,
	MultiplyWithScalar,
	DivideByScalar,
	DivideScalarBy,
	CompareEqual,
	CompareEqualScalar,
	Compare,
	CompareScalar,
	CreateSequence,
	InitializeWith,
	Memset,
	FillSequential,
	SetSlice,
	ResetSlice,
	GetSlice,
	Max,
	Min,
	RotateLeft,
	RotateRight,
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
#else
	Abs, /* Abs is defined only for signed/float types */
#endif
#ifdef __IS_INTEGER__
	Mod,
	ModScalar,
#else
	FCompare,
	Inverse,
#endif
	ForEach,
	Accumulate,
	Product,
	Fprintf,
	Select,
	SelectCopy,
	GetData,
	Back,
	Front,
	RemoveRange,
	Resize,
};
