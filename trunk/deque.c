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

struct deque_node_t {
    struct deque_node_t *next;
    struct deque_node_t *prev;
    char value[1];
};

struct deque_t {
    DequeInterface *VTable;
    size_t count;
    unsigned Flags;
    size_t ElementSize;
    struct deque_node_t *head;
    struct deque_node_t *tail;
    CompareFunction compare;
    ErrorFunction RaiseError;   /* Error function */
	ContainerMemoryManager *Allocator;
	unsigned timestamp;
};

typedef struct deque_node_t *DequeNode;

/* The default comparator which simplies does a simple comparison based on
* memory address.  It is really only useful to compare if two pointers point
* to the same piece of data, beyond that less than or equal are not very
* useful.
*/
static int default_comparator(const void *left,const void *right,CompareInfo *ExtraArgs)
{
    size_t siz=((Deque *)ExtraArgs->Container)->ElementSize;
    return memcmp(left,right,siz);
}


/* Create a deque and return a reference, if memory cannot be allocated for
* the deque, NULL will be returned.
*
*/
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))
static Deque * Init(Deque *d, size_t elementsize) 
{
	memset(d,0,sizeof(struct deque_t));
    /* store a pointer to the comparison function */
    d->compare = default_comparator;

    d->ElementSize = elementsize;
	d->VTable =&iDeque;
	d->Allocator = CurrentMemoryManager;
    return d;
}

static Deque * Create(size_t elementsize) 
{
    Deque * d = CurrentMemoryManager->malloc(sizeof(struct deque_t));
    if (d == NULL)	{
		iError.RaiseError("iDeque.Create",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	return Init(d,elementsize);
}


/* Free the data allocated for the deque and all nodes */
static int Finalize(Deque * d) 
{
    int r = iDeque.Clear(d);
	d->Allocator->free(d);
    return r;
}

/* Append the specified item to the right end of the deque (head).
*/
static int Add(Deque * d, void* item) 
{
    DequeNode newNode;
    assert(d != NULL);

    /* allocate memory for the new node and put it in a valid state */
    newNode = d->Allocator->malloc(sizeof(struct deque_node_t)+d->ElementSize);
	if (newNode == NULL) {
		iError.RaiseError("iDeque.Add",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
    newNode->prev = d->head;
    newNode->next = NULL;
    memcpy(newNode->value,item,d->ElementSize);

    if (d->head != NULL) {
        d->head->next = newNode;
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
    newNode = d->Allocator->malloc(sizeof(struct deque_node_t)+d->ElementSize);
    newNode->next = d->tail;
    newNode->prev = NULL;
    memcpy(newNode->value, item,d->ElementSize);

    if (d->tail != NULL) {
        d->tail->prev = newNode;
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
static int Clear(Deque * d) {
    DequeNode tmp;
    assert(d != NULL);
    while (d->head != NULL) {
        tmp = d->head;
        d->head = tmp->next;
		d->Allocator->free(tmp);
    }
    d->head = NULL;
    d->tail = NULL;
    d->count = 0;
    return 0;
}

/* Remove the rightmost element from the deque and return a reference to the
* value pointed to by the deque node.  If there is no rightmost element
* then NULL will be returned.
*
 * This operation is O(1), constant time.
*/
static int PopFront(Deque * d,void *outbuf) 
{
    DequeNode prevHead;
    void* value;
	
	if (d == NULL || outbuf == NULL) {
		iError.RaiseError("iDeque.PopBack",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
    if ((prevHead = d->head) == NULL) {
        return 0;
    }
    else {
        d->head = prevHead->prev;
        d->count--;
        value = prevHead->value;
		d->Allocator->free(prevHead);
        memcpy(outbuf,value,d->ElementSize);
		return 1;
    }
}

/* Get the value of the deque tail or NULL if the deque is empty */
static int PeekFront(Deque * d,void *outbuf) 
{
	if (d == NULL || outbuf == NULL) {
		iError.RaiseError("iDeque.PopBack",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	
    if (d->head == NULL) {
        return 0;
    }
    memcpy(outbuf,d->head->value,d->ElementSize);
	return 1;
}

/* Remove the leftmost element from the deque and return a reference to the
* value pointed to by the deque node.  If there is no leftmost element
* then NULL will be returned.
*
 * This operation is O(1), constant time.
*/
static int PopBack(Deque * d,void *outbuf) 
{
    DequeNode prevTail;
    void* value;

	if (d == NULL || outbuf == NULL) {
		iError.RaiseError("iDeque.PopBack",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	
    if (d->tail == NULL) {
        return 0;
    }
    else {
        prevTail = d->tail;
        d->tail = prevTail->next;
        if (d->tail != NULL) {
            d->tail->prev = NULL;
        }
        d->count--;
        value = prevTail->value;
		d->Allocator->free(prevTail);
        memcpy(outbuf,value,d->ElementSize);
		return 1;
    }
}

/* Get the value of the deque head (leftmost element) or NULL if empty */
static int PeekBack(Deque * d,void *outbuf) 
{
	if (d == NULL || outbuf == NULL) {
		iError.RaiseError("iDeque.PopBack",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
    if (d->tail == NULL) {
        return 0;
    }
    memcpy(outbuf,d->tail->value,d->ElementSize);
	return 1;
}

/* Remove the first occurrence of item from the deque, starting from the left.
* A reference to the removed node value will be returned, otherwise NULL
* will be returned (if the item cannot be found).
*
 * Note that the comparison is done on the values of the data pointers, so
* even if I had two strings "foo" and "foo" at different places in memory,
* we would not get a match.
*
 * This operation executes in O(n) time where n is the number of elements in
* the deque due to a linear search for the item.
*/
static int Remove(Deque * d, void* item) 
{
    DequeNode tmp = d->tail;
    CompareInfo ci;

    ci.Container = d;
    ci.ExtraArgs = NULL;

    while (tmp != NULL) {
        if ((d->compare)(tmp->value, item,&ci) == 0) {
            if (tmp->prev != NULL) {
                tmp->prev->next = tmp->next;
            }
            if (tmp->next != NULL) {
                tmp->next->prev = tmp->prev;
            }
			d->Allocator->free(tmp);
            d->count--;
            return 1;
        }
        tmp = tmp->next;
    }
    return 0; /* item not found in deque */
}

/* Return the number of items in the deque */
static size_t GetCount(Deque * d) {
    return d->count;
}

/* Copy the deque and return a reference to the new deque
*
 * This is a shallow copy, so only the deque container data structures are
* copied, not the values referenced.
*/
static Deque *Copy(Deque * d) {
    Deque * NewDeque;
    DequeNode tmp;
    NewDeque = Create(d->ElementSize);
    tmp = d->head;
    while (tmp != NULL) {
        Add(NewDeque, tmp->value);
        tmp = tmp->next;
    }
	NewDeque->Allocator = d->Allocator;
	NewDeque->compare = d->compare;
	NewDeque->Flags = NewDeque->Flags;
	NewDeque->RaiseError = d->RaiseError;
    return NewDeque;
}

/* Reverse the order of the items in the deque */
static int Reverse(Deque * d) 
{
    DequeNode currNode;
    DequeNode nextNode;
	
	if (d == NULL) {
		iError.RaiseError("iDeque.Reverse",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (d->count == 0)
		return 0;
    currNode = d->head;
    while (currNode != NULL) {
        nextNode = currNode->next;
        currNode->next = currNode->prev;
        currNode->prev = nextNode;
        currNode = nextNode;
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
    ci.Container = d;
    ci.ExtraArgs = NULL;

    pos=1;
    while (tmp != NULL) {
        if ((d->compare)(tmp->value, item,&ci) == 0) {
            return pos;
        }
        tmp = tmp->next;
        pos++;
    }
    return 0; /* item not found in deque */
}

static int Equal(Deque * d, Deque *d1) 
{
    DequeNode tmp = d->head;
	DequeNode tmp1 = d1->head;
    CompareInfo ci;
    ci.Container = d;
    ci.ExtraArgs = NULL;

	if (d->ElementSize != d1->ElementSize)
		return 0;
    while (tmp != NULL) {
        if ((d->compare)(tmp->value, tmp1->value,&ci) != 0) {
            return 0;
        }
        tmp = tmp->next;
		tmp1 = tmp1->next;
    }
    return 1; 
}

static void Apply(Deque * d, int (*Applyfn)(void *,void * arg),void *arg) 
{
    DequeNode tmp = d->head;

    while (tmp != NULL) {
        Applyfn(tmp->value, arg);
        tmp = tmp->next;
    }
}

static unsigned GetFlags(Deque *d) {
    return d->Flags;
}
static unsigned SetFlags(Deque *d,unsigned newflags){
    unsigned oldflags = d->Flags;
    d->Flags = newflags;
    return oldflags;
}
static int Save(Deque *d,FILE *stream, SaveFunction saveFn,void *arg)
{
	size_t i;
    DequeNode tmp = d->head;

	if (fwrite(d,1,sizeof(Deque),stream) <= 0)
		return EOF;
	if (saveFn == NULL) {
		for (i=0; i<d->count; i++) {
			fwrite(tmp->value,1,d->ElementSize,stream);
			tmp = tmp->next;
		}
	}
	else {
		for (i=0; i< d->count; i++) {
		
			if (saveFn(tmp->value,arg,stream) <= 0)
				return EOF;
			tmp = tmp->next;
		}
	}
	return 0;
}

static Deque *Load(FILE *stream, ReadFunction loadFn,void *arg)
{
	size_t i;
	char *buf;
	Deque D,*d;

	if (fread(&D,1,sizeof(Deque),stream) <= 0)
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
			if (fread(buf,1,D.ElementSize,stream) <= 0)
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
	size_t timestamp;
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
	li->Current = li->Current->next;
	li->index++;
	result = li->Current->value;
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
		while (rvp && i < li->index) {
			rvp = rvp->next;
			i++;
		}
	}
	li->Current = rvp;
	return rvp->value;
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
	return L->head->value;
}

static Iterator *newIterator(Deque *L)
{
	struct DequeIterator *result = MALLOC(L,sizeof(struct DequeIterator));
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

DequeInterface iDeque = {
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
    Add,
    AddLeft,
    Reverse,
    PopBack,
    PeekBack,
    PopFront,
    PeekFront,
	Create,
	Init,
};

