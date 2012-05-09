/*
double-ended queue
/dek/ (deque) A queue which can have items added or removed from either end.
The Knuth reference below reports that the name was coined by E. J. Schweppe.
[D. E. Knuth, "The Art of Computer Programming. Volume 1: Fundamental Algorithms", second
edition, Sections 2.2.1, 2.6, Addison-Wesley, 1973].

Parts of this code's algorithms have been adapted from
http://github.com/posborne/simpleds
*/
#include <string.h>
#include <assert.h>
#include "containers.h"
#include "ccl_internal.h"

struct deque_t {
    DequeInterface *VTable;
    size_t count;
    unsigned Flags;
    size_t ElementSize;
    DlistElement *head;
    DlistElement *tail;
    CompareFunction compare;
    ErrorFunction RaiseError;   /* Error function */
    ContainerAllocator *Allocator;
    unsigned timestamp;
    DestructorFunction DestructorFn;
};

typedef DlistElement *DequeNode;

static int default_comparator(const void *left,const void *right,CompareInfo *ExtraArgs)
{
    size_t siz=((Deque *)ExtraArgs->ContainerLeft)->ElementSize;
    return memcmp(left,right,siz);
}


/* Create a deque and return a reference, if memory cannot be allocated for
* the deque, NULL will be returned.
*
*/
static Deque * Init(Deque *d, size_t elementsize) 
{
    memset(d,0,sizeof(struct deque_t));
    /* store a pointer to the comparison function */
    d->compare = default_comparator;

    d->ElementSize = elementsize;
    d->VTable =&iDeque;
    d->Allocator = CurrentAllocator;
    return d;
}

static Deque * Create(size_t elementsize) 
{
    Deque * d = CurrentAllocator->malloc(sizeof(struct deque_t));
    if (d == NULL)	{
    	iError.RaiseError("iDeque.Create",CONTAINER_ERROR_BADARG);
    	return NULL;
    }
    return Init(d,elementsize);
}


/* Free the data allocated for the deque and all nodes */
static int Finalize(Deque * d) 
{
    unsigned Flags;
    int r;

    if (d == NULL) return iError.NullPtrError("iDeque.Finalize");
    Flags = d->Flags;
    r = iDeque.Clear(d);
    if (r < 0) return r;
    if (Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(d,CCL_FINALIZE,NULL,NULL);
    d->Allocator->free(d);
    return r;
}

/* Append the specified item to the right end of the deque (head).
*/
static int Add(Deque * d,const void* item) 
{
    DequeNode newNode;
    if (d == NULL)
        return iError.NullPtrError("iDeque.Add");

    if (d->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(d,CCL_ADD,item,NULL);

    /* allocate memory for the new node and put it in a valid state */
    newNode = d->Allocator->malloc(sizeof(DlistElement)+d->ElementSize);
    if (newNode == NULL) {
    	iError.RaiseError("iDeque.Add",CONTAINER_ERROR_BADARG);
    	return CONTAINER_ERROR_BADARG;
    }
    newNode->Previous = d->head;
    newNode->Next = NULL;
    memcpy(newNode->Data,item,d->ElementSize);

    if (d->head != NULL) {
        d->head->Next = newNode;
    }
    if (d->tail == NULL) {
        d->tail = newNode; /* only one item */
    }
    d->head = newNode;
    ++d->count;
    return 1;
}

/* Append the specified item to the left end of the deque (tail).
*/
static int AddLeft(Deque * d, void* item) {
    DequeNode newNode;
    assert(d != NULL);

    /* create the new node and put it in a valid state */
    newNode = d->Allocator->malloc(sizeof(DlistElement)+d->ElementSize);
    newNode->Next = d->tail;
    newNode->Previous = NULL;
    memcpy(newNode->Data, item,d->ElementSize);

    if (d->tail != NULL) {
        d->tail->Previous = newNode;
    }
    if (d->head == NULL) {
        d->head = d->tail;
    }
    d->tail = newNode;
    ++d->count;
    return 1;
}

/* Clear the specified deque, this will free all data structures related to the
* deque itself but will not free anything free the items being pointed to.
*
 * This operation is O(n) where n is the number of elements in the deque.
*/
static int Clear(Deque * d) 
{
    DequeNode tmp;

    if (d == NULL) {
        return iError.NullPtrError("iDeque.Clear");
    }
    if (d->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(d,CCL_CLEAR,NULL,NULL);


    while (d->head != NULL) {
        tmp = d->head;
        d->head = tmp->Next;
    	if (d->DestructorFn)
    		d->DestructorFn(tmp);
    	d->Allocator->free(tmp);
    }
    d->head = NULL;
    d->tail = NULL;
    d->count = 0;
    return 0;
}

/* Remove the rightmost element from the deque and return a reference to the
* Data pointed to by the deque node.  If there is no rightmost element
* then NULL will be returned.
*
 * This operation is O(1), constant time.
*/
static int PopFront(Deque * d,void *outbuf) 
{
    DequeNode PreviousHead;
    void* Data;
    
    if (d == NULL || outbuf == NULL) {
    	return iError.NullPtrError("iDeque.PopBack");
    }

    if ((PreviousHead = d->head) == NULL) {
        return 0;
    }
    else {
        d->head = PreviousHead->Previous;
        d->count--;
        Data = PreviousHead->Data;
    	if (d->DestructorFn)
    		d->DestructorFn(PreviousHead);
        if (d->Flags & CONTAINER_HAS_OBSERVER)
            iObserver.Notify(d,CCL_POP,Data,NULL);

    	d->Allocator->free(PreviousHead);
        memcpy(outbuf,Data,d->ElementSize);
    	return 1;
    }
}

/* Get the Data of the deque tail or NULL if the deque is empty */
static int PeekFront(Deque * d,void *outbuf) 
{
    if (d == NULL || outbuf == NULL) {
    	iError.RaiseError("iDeque.PopBack",CONTAINER_ERROR_BADARG);
    	return CONTAINER_ERROR_BADARG;
    }
    
    if (d->head == NULL) {
        return 0;
    }
    memcpy(outbuf,d->head->Data,d->ElementSize);
    return 1;
}

/* Remove the leftmost element from the deque and return a reference to the
* Data pointed to by the deque node.  If there is no leftmost element
* then NULL will be returned.
*
 * This operation is O(1), constant time.
*/
static int PopBack(Deque * d,void *outbuf) 
{
    DequeNode PreviousTail;
    void* Data;

    if (d == NULL || outbuf == NULL) {
    	iError.RaiseError("iDeque.PopBack",CONTAINER_ERROR_BADARG);
    	return CONTAINER_ERROR_BADARG;
    }
    
    if (d->tail == NULL) {
        return 0;
    }
    else {
        PreviousTail = d->tail;
        d->tail = PreviousTail->Next;
        if (d->tail != NULL) {
            d->tail->Previous = NULL;
        }
        d->count--;
        Data = PreviousTail->Data;
    	if (d->DestructorFn)
    		d->DestructorFn(PreviousTail);
    	d->Allocator->free(PreviousTail);
        memcpy(outbuf,Data,d->ElementSize);
    	return 1;
    }
}

/* Get the Data of the deque head (leftmost element) or NULL if empty */
static int PeekBack(Deque * d,void *outbuf) 
{
    if (d == NULL || outbuf == NULL) {
    	iError.RaiseError("iDeque.PopBack",CONTAINER_ERROR_BADARG);
    	return CONTAINER_ERROR_BADARG;
    }
    if (d->tail == NULL) {
        return 0;
    }
    memcpy(outbuf,d->tail->Data,d->ElementSize);
    return 1;
}

/* Remove the first occurrence of item from the deque, starting from the left.
* A reference to the removed node Data will be returned, otherwise NULL
* will be returned (if the item cannot be found).
*
 * Note that the comparison is done on the Datas of the data pointers, so
* even if I had two strings "foo" and "foo" at different places in memory,
* we would not get a match.
*
 * This operation executes in O(n) time where n is the number of elements in
* the deque due to a linear search for the item.
*/
static int EraseInternal(Deque * d,const void* item,int all) 
{
    DequeNode tmp = d->tail,deleted;
    CompareInfo ci;

    ci.ContainerLeft = d;
    ci.ExtraArgs = NULL;

    while (tmp != NULL) {
        if ((d->compare)(tmp->Data, item,&ci) == 0) {
            if (tmp->Previous != NULL) {
                tmp->Previous->Next = tmp->Next;
            }
            if (tmp->Next != NULL) {
                tmp->Next->Previous = tmp->Previous;
            }
    	    if (d->DestructorFn)
    	        d->DestructorFn(tmp);
            deleted = tmp;
            tmp = tmp->Previous;
    	    d->Allocator->free(deleted);
            d->count--;
            if (all == 0) return 1;
        }
        if (tmp)
            tmp = tmp->Next;
    }
    return 0; /* item not found in deque */
}

static int Erase(Deque * d,const void* item)
{
    return EraseInternal(d,item,0);
}

static int EraseAll(Deque * d,const void* item)
{
    return EraseInternal(d,item,1);
}

/* Return the number of items in the deque */
static size_t GetCount(Deque * d) {
    return d->count;
}

/* Copy the deque and return a reference to the new deque */
static Deque *Copy(Deque * d) 
{
    Deque * NewDeque;
    DequeNode tmp;

    if (d == NULL) {
        iError.NullPtrError("iDeque.Copy");
        return NULL;
    }
    NewDeque = Create(d->ElementSize);
    tmp = d->head;
    while (tmp != NULL) {
        Add(NewDeque, tmp->Data);
        tmp = tmp->Next;
    }
    NewDeque->Allocator = d->Allocator;
    NewDeque->compare = d->compare;
    NewDeque->Flags = NewDeque->Flags;
    NewDeque->RaiseError = d->RaiseError;

    if (d->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(d,CCL_COPY,NewDeque,NULL);

    return NewDeque;
}

/* Reverse the order of the items in the deque */
static int Reverse(Deque * d) 
{
    DequeNode currNode;
    DequeNode NextNode;
    
    if (d == NULL) {
    	return iError.NullPtrError("iDeque.Reverse");
    }
    if (d->count == 0)
    	return 0;
    currNode = d->head;
    while (currNode != NULL) {
        NextNode = currNode->Next;
        currNode->Next = currNode->Previous;
        currNode->Previous = NextNode;
        currNode = NextNode;
    }

    /* flip head and tail */
    currNode = d->tail;
    d->tail = d->head;
    d->head = currNode;
    return 1;
}

/* Return TRUE if the deque contains the specified item and FALSE if not */
static size_t Contains(Deque * d, void* item) 
{
    size_t pos;
    DequeNode tmp = d->head;
    CompareInfo ci;
    ci.ContainerLeft = d;
    ci.ExtraArgs = NULL;

    pos=1;
    while (tmp != NULL) {
        if (d->compare(tmp->Data, item,&ci) == 0) {
            return pos;
        }
        tmp = tmp->Next;
        pos++;
    }
    return 0; /* item not found in deque */
}

static int Equal(Deque * d, Deque *d1) 
{
    DequeNode tmp = d->head;
    DequeNode tmp1 = d1->head;
    CompareInfo ci;
    ci.ContainerLeft = d;
    ci.ExtraArgs = NULL;

    if (d->ElementSize != d1->ElementSize)
    	return 0;
    while (tmp != NULL && tmp1 != NULL) {
        if (d->compare(tmp->Data, tmp1->Data,&ci) != 0) {
            return 0;
        }
        tmp = tmp->Next;
    	tmp1 = tmp1->Next;
    }
    if (tmp != tmp1) return 0;
    return 1; 
}

static void Apply(Deque * d, int (*Applyfn)(void *,void * arg),void *arg) 
{
    DequeNode tmp = d->head;

    while (tmp != NULL) {
        Applyfn(tmp->Data, arg);
        tmp = tmp->Next;
    }
}

static unsigned GetFlags(Deque *d) {
    return d->Flags;
}
static unsigned SetFlags(Deque *d,unsigned newflags){
    unsigned oldflags = d->Flags;
    d->Flags = newflags;
	d->timestamp++;
    return oldflags;
}
static int Save(const Deque *d,FILE *stream, SaveFunction saveFn,void *arg)
{
    size_t i;
    DequeNode tmp = d->head;

    if (fwrite(d,1,sizeof(Deque),stream) == 0)
    	return EOF;
    if (saveFn == NULL) {
    	for (i=0; i<d->count; i++) {
    		if (0 == fwrite(tmp->Data,1,d->ElementSize,stream))
			return EOF;
    		tmp = tmp->Next;
    	}
    }
    else {
    	for (i=0; i< d->count; i++) {
    	
    		if (saveFn(tmp->Data,arg,stream) <= 0)
    			return EOF;
    		tmp = tmp->Next;
    	}
    }
    return 0;
}

static Deque *Load(FILE *stream, ReadFunction loadFn,void *arg)
{
    size_t i;
    char *buf;
    Deque D,*d;

    if (fread(&D,1,sizeof(Deque),stream) == 0)
    	return NULL;
    d = Create(D.ElementSize);
    if (d == NULL)
    	return NULL;
    buf = malloc(D.ElementSize);
    if (buf == NULL) {
    	Finalize(d);
    	iError.RaiseError("iDeque.Load",CONTAINER_ERROR_NOMEMORY);
    	return NULL;
    }
    for (i=0; i<D.count;i++) {
    	if (loadFn == NULL) {
    		if (fread(buf,1,D.ElementSize,stream) == 0)
    			break;
    	}
    	else {
    		if (loadFn(buf,arg,stream) <= 0) {
    			break;
    		}
    	}
    	Add(d,buf);
    }
    free(buf);
    d->count = D.count;
    d->Flags = D.Flags;
    return d;
}

static ErrorFunction SetErrorFunction(Deque *l,ErrorFunction fn)
{
    if (l == NULL) return iError.RaiseError;
    ErrorFunction old;
    old = l->RaiseError;
    l->RaiseError = (fn) ? fn : iError.EmptyErrorFunction;
    return old;
}

static size_t Sizeof(Deque *d)
{
    size_t result = sizeof(Deque);
    if (d == NULL)
    	return result;
    result += d->count*(sizeof(DequeNode)+d->ElementSize);
    return result;
}

struct DequeIterator {
    Iterator it;
    Deque *D;
    size_t index;
    DequeNode Current;
    unsigned timestamp;
    char ElementBuffer[1];
};

static void *GetNext(Iterator *it)
{
    struct DequeIterator *li = (struct DequeIterator *)it;
    Deque *D = li->D;
    void *result;

    if (li->index >= (D->count-1) || li->Current == NULL)
    	return NULL;
    if (li->timestamp != D->timestamp) {
    	D->RaiseError("GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
    	return NULL;
    }
    li->Current = li->Current->Next;
    li->index++;
    result = li->Current->Data;
    return result;
}

static void *GetPrevious(Iterator *it)
{
    struct DequeIterator *li = (struct DequeIterator *)it;
    Deque *L = li->D;
    DequeNode rvp;
    size_t i;

    if (li->index >= L->count || li->index == 0)
    	return NULL;
    if (li->timestamp != L->timestamp) {
    	L->RaiseError("GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
    	return NULL;
    }
    rvp = L->head;
    i=0;
    li->index--;
    if (li->index > 0) {
    	while (rvp != NULL && i < li->index) {
    		rvp = rvp->Next;
    		i++;
    	}
    }
    if (rvp == NULL) return NULL;
    li->Current = rvp;
    return rvp->Data;
}
static void *GetCurrent(Iterator *it)
{
    struct DequeIterator *li = (struct DequeIterator *)it;
    return li->Current;
}
static void *GetFirst(Iterator *it)
{
    struct DequeIterator *li = (struct DequeIterator *)it;
    Deque *L = li->D;
    if (L->count == 0)
    	return NULL;
    if (li->timestamp != L->timestamp) {
    	L->RaiseError("GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
    	return NULL;
    }
    li->index = 0;
    li->Current = L->head;
    return L->head->Data;
}

static Iterator *NewIterator(Deque *L)
{
    struct DequeIterator *result = L->Allocator->malloc(sizeof(struct DequeIterator));
    if (result == NULL)
    	return NULL;
    result->it.GetNext = GetNext;
    result->it.GetPrevious = GetPrevious;
    result->it.GetFirst = GetFirst;
    result->it.GetCurrent = GetCurrent;
    result->D = L;
    result->timestamp = L->timestamp;
    return &result->it;
}

static int InitIterator(Deque *L,void *buf)
{
    struct DequeIterator *result = buf;
    result->it.GetNext = GetNext;
    result->it.GetPrevious = GetPrevious;
    result->it.GetFirst = GetFirst;
    result->it.GetCurrent = GetCurrent;
    result->D = L;
    result->timestamp = L->timestamp;
    return 1;
}


static int deleteIterator(Iterator *it)
{
    struct DequeIterator *li;
    Deque *L;

    if (it == NULL) {
    	iError.RaiseError("deleteIterator",CONTAINER_ERROR_BADARG);
    	return CONTAINER_ERROR_BADARG;
    }
    li = (struct DequeIterator *)it;
    L = li->D;
    L->Allocator->free(it);
    return 1;
}

static DestructorFunction SetDestructor(Deque *cb,DestructorFunction fn)
{
    DestructorFunction oldfn;
    if (cb == NULL)
    	return NULL;
    oldfn = cb->DestructorFn;
    if (fn)
    	cb->DestructorFn = fn;
    return oldfn;
}

static size_t SizeofIterator(Deque *l)
{
	return sizeof(struct DequeIterator);
}


DequeInterface iDeque = {
    GetCount,
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
    Add,
    AddLeft,
    Reverse,
    PopBack,
    PeekBack,
    PopFront,
    PeekFront,
    Create,
    Init,
    SetDestructor,
};

