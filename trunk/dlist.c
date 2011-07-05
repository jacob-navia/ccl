#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif
#include "containers.h"
#include "ccl_internal.h"

static const guid DlistGuid = {0xac2525ff, 0x2e2a, 0x4540,
{0xae,0x70,0xc4,0x7a,0x2,0xf7,0xa,0xed}
};


/* The function Push is identical to Insert */
#define Push Insert

#define CONTAINER_READONLY	1
#define LIST_HASPOINTER	2
#define CHUNK_SIZE	1000
static int Add_nd(Dlist *l,void *elem);
/*------------------------------------------------------------------------
 Procedure:     new_link ID:1
 Purpose:       Allocation of a new Dlist element. If the element
                size is zero, we have an heterogenous
                Dlist, and we allocate just a pointer to the data
                that is maintained by the user.
                Note that we allocate the size of a Dlist element
                plus the size of the data in a single
                block. This block should be passed to the FREE
                function.

 Input:         The Dlist where the new element should be added and a
                pointer to the data that will be added (can be
                NULL).
 Output:        A pointer to the new Dlist element (can be NULL)
 Errors:        If there is no memory returns NULL
------------------------------------------------------------------------*/
static dlist_element *new_dlink(Dlist *l,void *data,const char *fname)
{
    dlist_element *result;

    if (l->Heap == NULL) {
    	result = l->Allocator->malloc(sizeof(*result)+l->ElementSize);
    }
    else result = iHeap.newObject(l->Heap);
    if (result == NULL) {
    	l->RaiseError(fname,CONTAINER_ERROR_NOMEMORY);
    }
    else {
    	result->Next = NULL;
    	result->Previous = NULL;
    	memcpy(&result->Data,data,l->ElementSize);
    }
    return result;
}

static int DefaultDlistCompareFunction(const void *left,const void *right,CompareInfo *ExtraArgs)
{
        size_t siz=((Dlist *)ExtraArgs->ContainerLeft)->ElementSize;
        return memcmp(left,right,siz);
}

static Dlist *Init(Dlist *result,size_t elementsize)
{
    if (elementsize == 0) {
    	iError.NullPtrError("iDlist.Create");
    	return NULL;
    }
    memset(result,0,sizeof(Dlist));
    result->ElementSize = elementsize;
    result->VTable = &iDlist;
    result->Compare = DefaultDlistCompareFunction;
    result->RaiseError = iError.RaiseError;
    result->Allocator = CurrentMemoryManager;
    return result;
}

/*------------------------------------------------------------------------
 Procedure:     Create ID:1
 Purpose:       Allocates a new Dlist object header, initializes the
                VTable field and the element size
 Input:         The size of the elements of the Dlist. Can be zero,
                meaning that the Dlist is made of objects managed by
                the user
 Output:        A pointer to the newly created Dlist or NULL if
                there is no memory.
 Errors:        If element size is smaller than zero an error
                routine is called. If there is no memory result is
                NULL.
------------------------------------------------------------------------*/
static Dlist *CreateWithAllocator(size_t elementsize,ContainerMemoryManager *allocator)
{
    Dlist *result,*r;

    if (elementsize == 0) {
    	iError.NullPtrError("iDlist.Create");
    	return NULL;
    }
    result = allocator->malloc(sizeof(Dlist));
    if (result == NULL) {
        iError.RaiseError("iDlist.Create",CONTAINER_ERROR_NOMEMORY);
    	return NULL;
    }
    r = Init(result,elementsize);
    if (r == NULL)
    	allocator->free(result);
    else r->Allocator = allocator;
    return r;
}

static Dlist *Create(size_t elementsize)
{
    return CreateWithAllocator(elementsize,CurrentMemoryManager);
}

static Dlist *InitializeWith(size_t elementSize,size_t n,void *Data)
{
        Dlist *result = Create(elementSize);
        size_t i;
        char *pData = Data;
        if (result == NULL)
                return result;
        for (i=0; i<n; i++) {
                Add_nd(result,pData);
                pData += elementSize;
        }
        return result;
}


static int UseHeap(Dlist *L, ContainerMemoryManager *m)
{
    if (L->Heap || L->count) {
    	L->RaiseError("iDlist.UseHeap",CONTAINER_ERROR_NOT_EMPTY);
    	return 0;
    }
    if (m == NULL)
    	m = L->Allocator;
    L->Heap = iHeap.Create(L->ElementSize+sizeof(dlist_element), m);
    if (L->Heap == NULL) {
    	L->RaiseError("iDlist.UseHeap",CONTAINER_ERROR_NOMEMORY);
    	return CONTAINER_ERROR_NOMEMORY;
    }
    return 1;

}

/* This function was proposed by Ben Pfaff in c.l.c if I remember correctly */
static Dlist *Splice ( Dlist *list, void *ppos, Dlist *toInsert, int dir )
{
    dlist_element *pos = ppos;
    if (( list == NULL ) || ( pos == NULL ) || toInsert == NULL) {
    	iError.NullPtrError("iDlist.Splice");
    	return NULL;
    }
    if (toInsert->count == 0)
    	return list;
    if ( dir == 0 ) {
    	toInsert->First->Previous = pos->Previous;
    	toInsert->Last->Next = pos;
    }
    else {
    	toInsert->First->Previous = pos;
    	toInsert->Last->Next = pos->Next;
    }
    if ( toInsert->First->Previous != NULL )
    	toInsert->First->Previous->Next = toInsert->First;
    if ( toInsert->Last->Next != NULL )
    	toInsert->Last->Next->Previous = toInsert->Last;
    list->count += toInsert->count;
    list->timestamp++;
    return list;
}



/*------------------------------------------------------------------------
 Procedure:     Clear ID:1
 Purpose:       Reclaims all memory used by a Dlist. The Dlist header
                itself is not reclaimed but zeroed. Note that the
                Dlist must be writable.
 Input:         The Dlist to be cleared
 Output:        Returns the number of elemnts that were in the Dlist
                if OK, CONTAINER_ERROR_READONLY otherwise.
 Errors:        None
------------------------------------------------------------------------*/
static int Clear(Dlist *l)
{

    if (l->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(l,CCL_CLEAR,NULL,NULL);

    if (l == NULL) {
    	return iError.NullPtrError("iDlist.Size");
    }
    if (l->Flags &CONTAINER_READONLY) {
    	l->RaiseError("iDlist.Add",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
#ifdef NO_GC
    if (l->Heap)
    	iHeap.Finalize(l->Heap);
    else {
    	dlist_element *rvp = l->First,*tmp;

    	while (rvp) {
    		tmp = rvp;
    		rvp = rvp->Next;
    		if (l->DestructorFn)
    			l->DestructorFn(tmp);
    		l->Allocator->free(tmp);
    	}
    }
#endif
    /* Clear the fields that need to be cleared but not all fields */
    l->count = 0;
    l->Heap = NULL;
    l->First = l->Last = NULL;
    l->Flags = 0;
    l->timestamp = 0;
    return 1;
}

/*------------------------------------------------------------------------
 Procedure:     Add ID:1
 Purpose:       Adds an element to the Dlist
 Input:         The Dlist and the elemnt to be added
 Output:        Returns the number of elements in the Dlist or a
                negative error code if an error occurs.
 Errors:        The element to be added can't be NULL, and the Dlist
                must be writable.
------------------------------------------------------------------------*/
static int Add_nd(Dlist *l,void *elem)
{
        dlist_element *newl = new_dlink(l,elem,"iList.Add");
        if (newl == 0)
                return CONTAINER_ERROR_NOMEMORY;
        if (l->count ==  0) {
                l->First = newl;
        }
        else {
                l->Last->Next = newl;
                newl->Previous = l->Last;
        }
        l->Last = newl;
        l->timestamp++;
        ++l->count;
    return 1;
}

static int Add(Dlist *l,void *elem)
{
    int r;
    if (elem == NULL || l == NULL) {
    	if (l)
    		l->RaiseError("iDlist.Add",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.Add");
    	return CONTAINER_ERROR_BADARG;
    }
    if (l->Flags &CONTAINER_READONLY) {
    	l->RaiseError("iDlist.Add",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    r = Add_nd(l,elem);
    if (r < 0)
    	return r;
    if (l->Flags & CONTAINER_HAS_OBSERVER)
    	iObserver.Notify(l,CCL_ADD,elem,NULL);

    return 1;
}

static int AddRange(Dlist * AL,size_t n, void *data)
{
        unsigned char *p;

        if (AL == NULL) {
                return iError.NullPtrError("iList.AddRange");
        }
        if (AL->Flags & CONTAINER_READONLY) {
                AL->RaiseError("iList.AddRange",CONTAINER_ERROR_READONLY);
                return CONTAINER_ERROR_READONLY;
        }
        if (data == NULL) {
                AL->RaiseError("iList.AddRange",CONTAINER_ERROR_BADARG);
                return CONTAINER_ERROR_BADARG;
        }
    	p = data;
        while (n > 0) {
                int r = Add_nd(AL,p);
                if (r < 0) {
                        return r;
                }
                p += AL->ElementSize;
        }
        AL->timestamp++;
    if (AL->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(AL,CCL_ADDRANGE,data,(void *)n);

        return 1;
}

/*------------------------------------------------------------------------
 Procedure:     SetReadOnly ID:1
 Purpose:       Sets/Unsets the read only flag.
 Input:         The Dlist and the new value
 Output:        The old value of the flag
 Errors:        None
------------------------------------------------------------------------*/
static unsigned SetFlags(Dlist *l,unsigned newval)
{
    int result;

    if (l == NULL) {
    	iError.NullPtrError("iDlist.Size");
    	return 0;
    }
    result = l->Flags;
    l->Flags = newval;
	l->timestamp++;
    return result;
}

/*------------------------------------------------------------------------
 Procedure:     IsReadOnly ID:1
 Purpose:       Queries the read only flag
 Input:         The Dlist
 Output:        The state of the flag
 Errors:        None
------------------------------------------------------------------------*/
static unsigned GetFlags(Dlist *l)
{
    if (l == NULL) {
    	iError.NullPtrError("iDlist.Size");
    	return 0;
    }
    return l->Flags;
}

/*------------------------------------------------------------------------
 Procedure:     Size ID:1
 Purpose:       Returns the number of elements in the Dlist
 Input:         The Dlist
 Output:        The number of elements
 Errors:        None
------------------------------------------------------------------------*/
static size_t Size(Dlist *l)
{
    if (l == NULL) {
    	iError.NullPtrError("iDlist.Size");
    	return 0;
    }
    return l->count;
}

/*------------------------------------------------------------------------
 Procedure:     SetCompareFunction ID:1
 Purpose:       Defines the function to be used in comparison of
                Dlist elements
 Input:         The Dlist and the new comparison function. If the new
                comparison function is NULL no changes are done.
 Output:        Returns the old function
 Errors:        None
------------------------------------------------------------------------*/
static CompareFunction SetCompareFunction(Dlist *l,CompareFunction fn)
{
    CompareFunction oldfn;
    

    if (l == NULL) {
    	iError.NullPtrError("iDlist.Size");
    	return NULL;
    }
    oldfn = l->Compare;
    if (fn != NULL) /* Treat NULL as an enquiry to get the compare function */
    	l->Compare = fn;
    return oldfn;
}

/*------------------------------------------------------------------------
 Procedure:     Copy ID:1
 Purpose:       Copies a Dlist. The result is fully allocated (Dlist
                elements and data).
 Input:         The Dlist to be copied
 Output:        A pointer to the new Dlist
 Errors:        None. Returns NULL if therfe is no memory left.
------------------------------------------------------------------------*/
static Dlist *Copy(Dlist *l)
{
    Dlist *result;
    dlist_element *rvp,*newRvp,*last;

    if (l == NULL) {
    	iError.NullPtrError("iDlist.Size");
    	return 0;
    }
    result = Create(l->ElementSize);
    if (result == NULL)
    	return NULL;
    last = rvp = l->First;
    while (rvp) {
    	newRvp = new_dlink(result,rvp->Data,"iDlist.Copy");
    	if (newRvp == NULL) {
    		if (l->RaiseError) 
    			l->RaiseError("iDlist.Copy",CONTAINER_ERROR_NOMEMORY);
    		else
    			iError.RaiseError("iDlist.Copy",CONTAINER_ERROR_NOMEMORY);
    		iDlist.Finalize(result);
                	return NULL;
    	}
    	if (result->First == NULL)
    		last = result->First = newRvp;
    	else {
    		last->Next = newRvp;
    		newRvp->Previous = last;
    		last = newRvp;
    	}
    	result->Last = newRvp;
    	result->count++;
    	rvp = rvp->Next;
    }
    result->Flags = l->Flags;
    result->RaiseError = l->RaiseError;
    result->Compare = l->Compare;
    result->VTable = l->VTable;
    if (l->Flags & CONTAINER_HAS_OBSERVER)
    	iObserver.Notify(l,CCL_COPY,result,NULL);

    return result;
}

/*------------------------------------------------------------------------
 Procedure:     Finalize ID:1
 Purpose:       Reclaims all memory for a Dlist. The data, Dlist
                elements and the Dlist header are reclaimed.
 Input:         The Dlist to be destroyed. It should NOT be read-
                only.
 Output:        Returns the old count or a negative value if an
                error occurs (Dlist not writable)
 Errors:        Needs a writable Dlist
------------------------------------------------------------------------*/
static int Finalize(Dlist *l)
{
    int t;
    unsigned Flags;

    if (l == NULL) return iError.NullPtrError("iDlist.Finalize");
    
    Flags = l->Flags;
    t = Clear(l);
    if (Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(l,CCL_FINALIZE,NULL,NULL);
    if (t < 0)
    	return t;
    l->Allocator->free(l);
    return 1;
}

/*------------------------------------------------------------------------
 Procedure:     GetElement ID:1
 Purpose:       Returns the data associated with a given position
 Input:         The Dlist and the position
 Output:        A pointer to the data
 Errors:        NULL if error in the positgion index
------------------------------------------------------------------------*/
static void * GetElement(Dlist *l,int position)
{
    dlist_element *rvp;


    if (l == NULL) {
    	iError.NullPtrError("iDlist.ReplaceAt");
    	return 0;
    }
    if (position >= (signed)l->count || position < 0) {
    	l->RaiseError("GetElement",CONTAINER_ERROR_INDEX);
    	return NULL;
    }
    if (l->Flags & CONTAINER_READONLY) {
    	l->RaiseError("iDlist.GetElement",CONTAINER_ERROR_READONLY);
    	return NULL;
    }
    rvp = l->First;
    while (position) {
    	rvp = rvp->Next;
    	position--;
    }
    return rvp->Data;
}

static int ReplaceAt(Dlist *l,size_t position,void *data)
{
    dlist_element *rvp;

    if (l == NULL || data == NULL) {
    	if (l)
    		l->RaiseError("iDlist.ReplaceAt",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.ReplaceAt");
    	return CONTAINER_ERROR_BADARG;
    }
    /* Error checking */
    if (position >= l->count) {
    	l->RaiseError("iDlist.ReplaceAt",CONTAINER_ERROR_INDEX);
    	return CONTAINER_ERROR_INDEX;
    }
    if (l->Flags &CONTAINER_READONLY) {
    	l->RaiseError("iDlist.ReplaceAt",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    /* Position at the right data item */
    if (position == l->count-1)
    	rvp = l->Last;
    else  {
    	rvp = l->First;
    	while (position) {
    		rvp = rvp->Next;
    		position--;
    	}
    }
    if (l->DestructorFn)
    	l->DestructorFn(&rvp->Data);	
    /* Replace the data there */
    memcpy(&rvp->Data , data,l->ElementSize);
    l->timestamp++;
    return 1;
}

/*------------------------------------------------------------------------
 Procedure:     GetRange ID:1
 Purpose:       Gets a consecutive sub sequence of the Dlist within
                two indexes.
 Input:         The Dlist, the subsequence start and end
 Output:        A new freshly allocated Dlist. If the source Dlist is
                empty or the range is empty, the result Dlist is
                empty too.
 Errors:        None
------------------------------------------------------------------------*/
static Dlist *GetRange(Dlist *l,size_t start,size_t end)
{
    size_t counter;
    Dlist *result;
    dlist_element *rvp;

    if (l == NULL) {
    	iError.NullPtrError("iDlist.GetRange");
    	return 0;
    }
    result = Create(l->ElementSize);
    result->VTable = l->VTable;
    if (l->count == 0)
    	return result;
    if (end >= l->count)
    	end = l->count-1;
    if (start > end)
    	return result;
    if (start == l->count-1)
    	rvp = l->Last;
    else {
    	rvp = l->First;
    	counter = 0;
    	while (counter < start) {
    		rvp = rvp->Next;
    		counter++;
    	}
    }
    while (start <= end && rvp != NULL) {
    	int r = Add_nd(result,&rvp->Data);
    	if (r < 0) {
    		Finalize(result);
    		return NULL;
    	}
    	rvp = rvp->Next;
    	start++;
    }
    return result;
}

/*------------------------------------------------------------------------
 Procedure:     Equal ID:1
 Purpose:       Compares the data of two lists. They must be of the
                same length, the elements must have the same size
                and the comparison functions must be the same. If
                all this is true, the comparisons are done.
 Input:         The two lists to be compared
 Output:        Returns 1 if the two lists are equal, zero otherwise
 Errors:        None
------------------------------------------------------------------------*/
static bool Equal(Dlist *l1,Dlist *l2)
{
    dlist_element *link1,*link2;
    CompareInfo ci;
    CompareFunction fn;

    if (l1 == l2)
    	return 1;
    if (l1 == NULL || l2 == NULL)
    	return 0;
    if (l1->count != l2->count)
    	return 0;
    if (l1->ElementSize != l2->ElementSize)
    	return 0;
    if (l1->count == 0)
    	return 1;
    if (l1->Compare != l2->Compare)
    	return 0;
    fn = l1->Compare;
    link1 = l1->First;
    link2 = l2->First;
    ci.ContainerLeft = l1;
    ci.ContainerRight = l2;
    ci.ExtraArgs = NULL;
    while (link1 && link2) {
    	if (fn(link1->Data,link2->Data,&ci))
    		return 0;
    	link1 = link1->Next;
    	link2 = link2->Next;
    }
    if (link1 || link2)
    	return 0;
    return 1;
}


/*------------------------------------------------------------------------
 Procedure:     Insert ID:1
 Purpose:       Inserts an element at the start of the Dlist
 Input:         The Dlist and the data to insert
 Output:        The count of the Dlist after insertion, or CONTAINER_ERROR_READONLY
                if the Dlist is not writable or CONTAINER_ERROR_NOMEMORY if there is
    			no more memory left.
 Errors:        None
------------------------------------------------------------------------*/
static int PushFront(Dlist *l,void *pdata)
{
    dlist_element *rvp;

    if (l == NULL || pdata == NULL) {
    	if (l)
    		l->RaiseError("iDlist.PushFront",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.PushFront");
    	return CONTAINER_ERROR_BADARG;
    }
    if (l->Flags & CONTAINER_READONLY) {
    	l->RaiseError("iDlist.PushFront",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    rvp = new_dlink(l,pdata,"Insert");
    if (rvp == NULL)
    	return CONTAINER_ERROR_NOMEMORY;
    rvp->Next = l->First;
    rvp->Previous = NULL;
    if (l->First) {
    	l->First->Previous = rvp;
    }
    l->First = rvp;
    if (l->Last == NULL)
    	l->Last = rvp;
    l->count++;
    l->timestamp++;
    if (l->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(l,CCL_PUSH,pdata,NULL);

    return 1;
}

static int PushBack(Dlist *l,void *pdata)
{
    dlist_element *rvp;

    if (l == NULL) {
    	iError.NullPtrError("iDlist.Size");
    	return 0;
    }
    if (l->Flags & CONTAINER_READONLY) {
    	l->RaiseError("iDlist.PushBack",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    rvp = new_dlink(l,pdata,"iDlist.Insert");
    if (rvp == NULL)
    	return CONTAINER_ERROR_NOMEMORY;
    if (l->count == 0)
    	l->Last = l->First = rvp;
    else {
    	l->Last->Next = rvp;
    	rvp->Previous = l->Last;
    	l->Last = rvp;
    }
    l->count++;
    l->timestamp++;
    if (l->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(l,CCL_PUSH,pdata,NULL);

    return 1;
}


/*------------------------------------------------------------------------
 Procedure:     Pop ID:1
 Purpose:       Takes out the first element of a Dlist.
 Input:         The Dlist
 Output:        The first element or NULL if there is none or the
                Dlist is read only
 Errors:        None
------------------------------------------------------------------------*/
static int PopFront(Dlist *l,void *result)
{
    dlist_element *le;

    if (l == NULL) return iError.NullPtrError("iDlist.PopFront");
    
    if (l->count == 0)
    	return 0;
    if (l->Flags & CONTAINER_READONLY) {
    	l->RaiseError("iDlist.PopFront",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    le = l->First;
    if (l->count == 1) {
    	l->First = l->Last = NULL;
    }
    else {
    	l->First = l->First->Next;
    	l->First->Previous = NULL;
    }
    l->count--;
    if (result)
    	memcpy(result,&le->Data,l->ElementSize);
    le->Next = l->FreeList;
    l->FreeList = le;
    l->timestamp++;
    if (l->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(l,CCL_POP,result,NULL);

    return 1;
}

static int PopBack(Dlist *l,void *result)
{
    dlist_element *le;

    if (l == NULL) return iError.NullPtrError("iDlist.PopBack");
    
    if (l->count == 0)
    	return 0;
    if (l->Flags & CONTAINER_READONLY) {
    	l->RaiseError("iDlist.PopBack",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    le = l->Last;
    if (l->count == 1) {
    	l->First = l->Last = NULL;
    }
    else {
    	l->Last = le->Previous;
    	l->Last->Next = NULL;
    }
    l->count--;
    if (result)
    	memcpy(result,&le->Data,l->ElementSize);
    le->Next = l->FreeList;
    l->FreeList = le;
    l->timestamp++;
    return 1;
}

static int InsertAt(Dlist *l,size_t pos,void *pdata)
{
    dlist_element *elem;

    if (l == NULL || pdata == NULL) {
    	if (l)
    		l->RaiseError("iDlist.InsertAt",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.InsertAt");
    	return CONTAINER_ERROR_BADARG;
    }
    if (pos > l->count) {
    	l->RaiseError("iDlist.InsertAt",CONTAINER_ERROR_INDEX);
    	return CONTAINER_ERROR_INDEX;
    }
    if (l->Flags & CONTAINER_READONLY) {
    	l->RaiseError("iDlist.PushBack",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    if (pos == l->count) {
    	return l->VTable->Add(l,pdata);
    }
    elem = new_dlink(l,pdata,"iDlist.InsertAt");
    if (elem == NULL)
    	return CONTAINER_ERROR_NOMEMORY;
    if (pos == 0 || l->First == NULL) {
    	elem->Next = l->First;
    	if (l->First != NULL)
    		l->First->Previous = elem;
    	l->First = elem;
    }
    else {
    	dlist_element *rvp = l->First;
    	while (pos > 0) {
    		rvp = rvp->Next;
    		pos--;
    	}
    	elem->Next = rvp;
    	if (rvp->Previous)
    		rvp->Previous->Next = elem;
    	elem->Previous = rvp->Previous;
    	rvp->Previous = elem;
    }
    l->timestamp++;
    ++l->count;
    if (l->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(l,CCL_INSERT_AT,pdata,(void *)pos);

    return 1;
}

static int InsertIn(Dlist *l, size_t idx,Dlist *newData)
{
    size_t newCount;
    dlist_element *le,*nle;

    if (l == NULL || newData == NULL) {
    	if (l)
    		l->RaiseError("iDlist.InsertIn",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.InsertIn");
    	return CONTAINER_ERROR_BADARG;
    }
    if (l->Flags & CONTAINER_READONLY) {
    	l->RaiseError("iDlist.InsertIn",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    if (idx > l->count) {
    	l->RaiseError("iDlist.InsertIn",CONTAINER_ERROR_INDEX);
    	return CONTAINER_ERROR_INDEX;
    }
    if (l->ElementSize != newData->ElementSize) {
    	l->RaiseError("iDlist.InsertIn",CONTAINER_ERROR_INCOMPATIBLE);
    	return CONTAINER_ERROR_INCOMPATIBLE;
    }
    if (newData->count == 0)
    	return 1;
    newCount = l->count + newData->count;
    newData = Copy(newData);
    if (newData == NULL) {
    	return CONTAINER_ERROR_NOMEMORY;
    }
    if (l->count == 0) {
    	l->First = newData->First;
    	l->Last = newData->Last;
    }
    else {
    	le = l->First;
    	while (idx > 1) {
    		le = le->Next;
    		idx--;
    	}
    	nle = le->Next;
    	le->Next = newData->First;
    	newData->First->Previous = le;
    	newData->Last->Next = nle;
    	nle->Previous = newData->Last;
    }
    newData->Allocator->free(newData);
    l->timestamp++;
    l->count = newCount;
    if (l->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(l,CCL_INSERT_IN,newData,NULL);

    return 1;
}

static int EraseAt(Dlist *l,size_t position)
{
    dlist_element *rvp=NULL,*last;
    void *removed=NULL;

    if (l == NULL) return iError.NullPtrError("iDlist.InsertIn");
    
    if (position >= l->count) {
    	l->RaiseError("iDlist.EraseAt",CONTAINER_ERROR_INDEX);
    	return CONTAINER_ERROR_INDEX;
    }
    if (l->Flags & CONTAINER_READONLY) {
    	l->RaiseError("iDlist.PushBack",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    rvp = l->First;
    if (position == 0) {
    	if (l->First == l->Last) {
    		l->First = l->Last = NULL;
    	}
    	else {
    		l->First = l->First->Next;
    		l->First->Previous = NULL;
    	}
    }
    else if (position == l->count - 1) {
    	l->Last = l->Last->Previous;
    	l->Last->Next = NULL;
    }
    else {
    	last = l->First;
    	while (position > 0) {
    		last = rvp;
    		rvp = rvp->Next;
    		position --;
    	}
    	last->Next = rvp->Next;
    	last->Next->Previous = last;
    }
    if (rvp)
    	removed = rvp->Data;
    if (l->Heap && rvp) {
    	iHeap.AddToFreeList(l->Heap,rvp);
    }
    else if  (rvp) l->Allocator->free(rvp);
    l->timestamp++;
    --l->count;
    if (removed && (l->Flags & CONTAINER_HAS_OBSERVER))
        iObserver.Notify(l,CCL_ERASE_AT,removed,(void *)position);

    return 1;
}


static int Reverse(Dlist *l)
{
    dlist_element *tmp,*rvp;

    if (l == NULL)  {
		return iError.NullPtrError("iDlist.Reverse");
	}
    if (l->Flags & CONTAINER_READONLY) {
    	l->RaiseError("iDlist.Reverse",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    if (l->count < 2)
    	return 1;
    rvp = l->Last;
    l->Last = l->First;
    l->First = rvp;
    while (rvp) {
    	tmp = rvp->Previous;
    	rvp->Previous = rvp->Next;
    	rvp->Next = tmp;
        rvp = tmp;
    }
    l->Last->Next = NULL;
    if (l->First)
    	l->First->Previous = NULL;
    l->timestamp++;
    return 1;
}



/* Searches a Dlist for a given data item
   Returns a positive integer if found, negative if the end is reached
*/
static int IndexOf(Dlist *l,void *ElementToFind, void * args,size_t *result)
{
    dlist_element *rvp;
    int r,i=0;
    CompareFunction fn;
    CompareInfo ci;

    if (l == NULL || ElementToFind == NULL) {
    	if (l)
    		l->RaiseError("iDlist.IndexOf",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.IndexOf");
    	return CONTAINER_ERROR_BADARG;
    }
    rvp = l->First;
    fn = l->Compare;
    ci.ContainerLeft = l;
    ci.ContainerRight = NULL;
    ci.ExtraArgs = args;
    while (rvp) {
    	r = fn(&rvp->Data,ElementToFind,&ci);
    	if (r == 0) {
    		*result = i;
    		return 1;
    	}
    	rvp = rvp->Next;
    	i++;
    }
    return CONTAINER_ERROR_NOTFOUND;
}

static int Erase(Dlist *l,void *elem)
{
    size_t idx;
    int i;

    if (l == NULL || elem == NULL) {
    	if (l)
    		l->RaiseError("iDlist.Erase",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.Erase");
    	return CONTAINER_ERROR_BADARG;
    }
    i = IndexOf(l,elem,NULL,&idx);
    if (i < 0)
    	return i;
    return EraseAt(l,idx);
}

/*------------------------------------------------------------------------
 Procedure:     Contains ID:1
 Purpose:       Determines if the given data is in the container
 Input:         The Dlist and the data to be searched
 Output:        Returns 1 (true) if the data is in there, false
 otherwise
 Errors:        The same as the function IndexOf
 ------------------------------------------------------------------------*/
static int Contains(Dlist *l,void *data)
{
    size_t r;
    int i;
    if (l == NULL || data == NULL) {
    	if (l)
    		l->RaiseError("iDlist.Contains",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.Contains");
    	return CONTAINER_ERROR_BADARG;
    }
    i = IndexOf(l,data,NULL,&r);
    return (i < 0) ? 0 : 1;
}

static int CopyElement(Dlist *l,size_t position,void *outBuffer)
{
    dlist_element *rvp;

    if (l == NULL || outBuffer == NULL) {
    	if (l)
    		l->RaiseError("iDlist.CopyElement",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.CopyElement");
    	return CONTAINER_ERROR_BADARG;
    }
    if (position >= l->count) {
    	l->RaiseError("iDlist.CopyElement",CONTAINER_ERROR_INDEX);
    	return CONTAINER_ERROR_INDEX;
    }
    if (position < l->count/2) {
    	rvp = l->First;
    	while (position) {
    		rvp = rvp->Next;
    		position--;
    	}
    }
    else {
    	rvp = l->Last;
    	position = l->count - position;
    	position--;
    	while (position > 0) {
    		rvp = rvp->Previous;
    		position--;
    	}
    }
    memcpy(outBuffer,rvp->Data,l->ElementSize);
    return 1;
}


static int dlcompar (const void *elem1, const void *elem2,CompareInfo *ExtraArgs)
{
    dlist_element *Elem1 = *(dlist_element **)elem1;
    dlist_element *Elem2 = *(dlist_element **)elem2;
    Dlist *l = (Dlist *)ExtraArgs->ContainerLeft;
    CompareFunction fn = l->Compare;
    return fn(Elem1->Data,Elem2->Data,ExtraArgs);
}

static int Sort(Dlist *l)
{
    dlist_element **tab;
    size_t i;
    dlist_element *rvp;
    CompareInfo ci;

    if (l == NULL) return iError.NullPtrError("iDlist.Sort");
    
    if (l->Flags&CONTAINER_READONLY) {
    	l->RaiseError("iDlist.Sort",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    if (l->count < 2)
    	return 1;
    tab = l->Allocator->malloc(l->count * sizeof(dlist_element *));
    if (tab == NULL) {
    	l->RaiseError("iDlist.Sort",CONTAINER_ERROR_NOMEMORY);
    	return 0;
    }
    rvp = l->First;
    for (i=0; i<l->count;i++) {
    	tab[i] = rvp;
    	rvp = rvp->Next;
    }
    ci.ContainerLeft = l;
    ci.ContainerRight = NULL;
    ci.ExtraArgs = NULL;
    qsortEx(tab,l->count,sizeof(dlist_element *),dlcompar,&ci);
    for (i=1; i<l->count-1;i++) {
    	tab[i]->Next = tab[i+1];
    	tab[i]->Previous = tab[i-1];
    }
    tab[0]->Next = tab[1];
    tab[0]->Previous = NULL;
    tab[l->count-1]->Next = NULL;
    tab[l->count-1]->Previous = tab[l->count-2];
    l->Last = tab[l->count-1];
    l->First = tab[0];
    l->timestamp++;
    return 1;
}
static int Apply(Dlist *L,int (Applyfn)(void *,void *),void *arg)
{
    dlist_element *le;

    if (L == NULL)  return iError.NullPtrError("iDist.Apply");
    if (Applyfn == NULL) {
    	L->RaiseError("iDlist.Apply",CONTAINER_ERROR_BADARG);
    	return CONTAINER_ERROR_BADARG;
    }
    le = L->First;
    while (le) {
    	Applyfn(le->Data,arg);
    	le = le->Next;
    }
    return 1;
}

static ErrorFunction SetErrorFunction(Dlist *l,ErrorFunction fn)
{
    ErrorFunction old;

    if (l == NULL) {
    	iError.NullPtrError("iDlist.SetErrorFunction");
    	return NULL;
    }
    old = l->RaiseError;
    if (fn)
    	l->RaiseError = fn;
    return old;
}

/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */

static void *GetNext(Iterator *it)
{
    struct DListIterator *li = (struct DListIterator *)it;
    Dlist *L = li->L;
    void *result;

    if (li->index >= L->count || li->Current == NULL)
    	return NULL;
    if (li->timestamp != L->timestamp) {
    	L->RaiseError("iDlist.GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
    	return NULL;
    }
    if (L->Flags & CONTAINER_READONLY) {
    	memcpy(li->ElementBuffer,li->Current->Data,L->ElementSize);
    	result = li->ElementBuffer;
    }	
    else result = li->Current->Data;
    li->Current = li->Current->Next;
    li->index++;
    return result;
}

static void *GetPrevious(Iterator *it)
{
    struct DListIterator *li = (struct DListIterator *)it;
    Dlist *L = li->L;
    void *result;

    if (li->index >= L->count || li->index == 0)
    	return NULL;
    if (li->timestamp != L->timestamp) {
    	L->RaiseError("iDlist.GetPrevious",CONTAINER_ERROR_OBJECT_CHANGED);
    	return NULL;
    }
    result = li->Current->Data;
    li->Current = li->Current->Previous;
    li->index--;
    return result;
}

static void *GetFirst(Iterator *it)
{
    struct DListIterator *li = (struct DListIterator *)it;
    Dlist *L = li->L;
    if (L->count == 0)
    	return NULL;
    if (li->timestamp != L->timestamp) {
    	L->RaiseError("iDlist.GetFirst",CONTAINER_ERROR_OBJECT_CHANGED);
    	return NULL;
    }
    li->index = 1;
    li->Current = L->First->Next;
    if (L->Flags & CONTAINER_READONLY) {
    	memcpy(li->ElementBuffer,L->First->Data,L->ElementSize);
    	return li->ElementBuffer;
    }	
    return L->First->Data;
}

static int ReplaceWithIterator(Iterator *it, void *data,int direction) 
{
    struct DListIterator *li = (struct DListIterator *)it;
	int result;
	size_t pos;
	
	if (it == NULL) {
		iError.RaiseError("Replace",CONTAINER_ERROR_BADARG);
		return 0;
	}
	if (li->L->Flags & CONTAINER_READONLY) {
		li->L->RaiseError("Replace",CONTAINER_ERROR_READONLY);
		return CONTAINER_ERROR_READONLY;
	}	
	if (li->L->count == 0)
		return 0;
    if (li->timestamp != li->L->timestamp) {
        li->L->RaiseError("Replace",CONTAINER_ERROR_OBJECT_CHANGED);
        return CONTAINER_ERROR_OBJECT_CHANGED;
    }
	pos = li->index;
	if (direction)
		GetNext(it);
	else
		GetPrevious(it);
	if (data == NULL)
		result = EraseAt(li->L,pos);
	else {
		result = ReplaceAt(li->L,li->index,data);
	}
	if (result >= 0) {
		li->timestamp = li->L->timestamp;
	}
	return result;
}


static void *GetCurrent(Iterator *it)
{
    struct DListIterator *li = (struct DListIterator *)it;
    
    if (li == NULL) {
        iError.NullPtrError("iDlist.GetCurrent");
        return NULL;
    }
    if (li->L->count == 0)
    	return NULL;
    if (li->index == (size_t)-1) {
    	li->L->RaiseError("GetCurrent",CONTAINER_ERROR_BADARG);
    	return NULL;
    }
    if (li->L->Flags & CONTAINER_READONLY) {
    	return li->ElementBuffer;
    }
    return li->Current->Data;
}

static void *Seek(Iterator *it,size_t idx)
{
    struct DListIterator *li = (struct DListIterator *)it;
    dlist_element *rvp;

        if (it == NULL) {
                iError.NullPtrError("iDlist.Seek");
                return NULL;
        }
        if (li->L->count == 0)
                return NULL;
        rvp = li->L->First;
        if (idx == 0) {
        li->index = 0;
        li->Current = li->L->First;
        }
    else if (idx >= li->L->count-1) {
        li->index = li->L->count-1;
        li->Current = li->L->Last;
    }
    else {
        li->index = idx;
        while (idx > 0) {
            rvp = rvp->Next;
            idx--;
        }
        li->Current = rvp;
    }
    return li->Current;
}


static Iterator *NewIterator(Dlist *L)
{
    struct DListIterator *result;
    
    if (L == NULL) {
    	iError.NullPtrError("iDlist.NewIterator");
    	return NULL;
    }
    result = L->Allocator->malloc(sizeof(struct DListIterator));
    if (result == NULL) {
    	L->RaiseError("iDlist.NewIterator",CONTAINER_ERROR_NOMEMORY);
    	return NULL;
    }
    result->it.GetNext = GetNext;
    result->it.GetPrevious = GetPrevious;
    result->it.GetFirst = GetFirst;
    result->it.GetCurrent = GetCurrent;
    result->it.Seek = Seek;
	result->it.Replace = ReplaceWithIterator;
    result->L = L;
    result->timestamp = L->timestamp;
    return &result->it;
}
static int Append(Dlist *l1, Dlist *l2)
{

    if (l1 == NULL || l2 == NULL)  return iError.NullPtrError("iDlist.Append");
    
    if ((l1->Flags & CONTAINER_READONLY) || (l2->Flags & CONTAINER_READONLY)) {
    	l1->RaiseError("iDlist.Append",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    if (l2->ElementSize != l1->ElementSize) {
    	l1->RaiseError("iDlist.Append",CONTAINER_ERROR_INCOMPATIBLE);
    	return CONTAINER_ERROR_INCOMPATIBLE;
    }
    if (l1->count == 0) {
    	l1->First = l2->First;
    	l1->Last = l2->Last;
    }
    else if (l2->count > 0) {
    	l1->Last->Next = l2->First;
    	l2->First->Previous = l1->Last;
    	l1->Last = l2->Last;
    }
    l1->count += l2->count;
    l1->timestamp++;
    if (l1->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(l1,CCL_APPEND,l2,NULL);
    if (l2->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(l2,CCL_FINALIZE,NULL,NULL);

    l2->Allocator->free(l2);
    return 1;
}


static size_t GetElementSize(Dlist *d) 
{ 
    if (d == NULL) {
    	iError.NullPtrError("iDlist.GetElementSize");
    	return 0;
    }

    return d->ElementSize;
}
static size_t Sizeof(Dlist *dl)
{
    if (dl == NULL) {
    	return sizeof(Dlist);
    }
    return sizeof(Dlist) + dl->count * (sizeof(dlist_element)+dl->ElementSize);
}

static int deleteIterator(Iterator *it)
{
    struct DListIterator *li = (struct DListIterator *)it;
    Dlist *L;
    
    if (it == NULL) return iError.NullPtrError("iDlist.DeleteIterator");
    
    L = li->L;
    L->Allocator->free(it);
    return 1;
}
static size_t DefaultSaveFunction(const void *element,void *arg, FILE *Outfile)
{
    const unsigned char *str = element;
    size_t len = *(size_t *)arg;

    return fwrite(str,1,len,Outfile);
}

static int Save(Dlist *L,FILE *stream, SaveFunction saveFn,void *arg)
{
    size_t i;
    dlist_element *rvp;

    if (stream == NULL || L == NULL ) {
    	if (L)
    		L->RaiseError("iDlist.Save",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.Save");
    	return CONTAINER_ERROR_BADARG;
    }
    if (saveFn == NULL) {
    	saveFn = DefaultSaveFunction;
    	arg = &L->ElementSize;
    }
    if (fwrite(&DlistGuid,sizeof(guid),1,stream) <= 0)
    	return EOF;
    if (fwrite(L,1,sizeof(Dlist),stream) <= 0)
    	return EOF;
    rvp = L->First;
    for (i=0; i< L->count; i++) {
    	char *p = rvp->Data;

    	if (saveFn(p,arg,stream) <= 0)
    		return EOF;
    	rvp = rvp->Next;
    }
    return 1;
}

static size_t DefaultLoadFunction(void *element,void *arg, FILE *Infile)
{
    size_t len = *(size_t *)arg;

    return fread(element,1,len,Infile);
}

static Dlist *Load(FILE *stream, ReadFunction loadFn,void *arg)
{
    size_t i;
    Dlist *result,L;
    dlist_element *newl;
    char *buf;
    guid Guid;

    if (stream == NULL) {
    	iError.NullPtrError("iDlist.Load");
    	return NULL;
    }
    if (loadFn == NULL) {
    	loadFn = DefaultLoadFunction;
    	arg = &L.ElementSize;
    }
    if (fread(&Guid,sizeof(guid),1,stream) <= 0) {
    	iError.RaiseError("iDlist.Load",CONTAINER_ERROR_FILE_READ);
    	return NULL;
    }
    if (memcmp(&Guid,&DlistGuid,sizeof(guid))) {
    	iError.RaiseError("iDlist.Load",CONTAINER_ERROR_WRONGFILE);
    	return NULL;
    }
    if (fread(&L,1,sizeof(Dlist),stream) <= 0) {
    	return NULL;
    }
    buf = malloc(L.ElementSize);
    if (buf == NULL) {
    	iError.RaiseError("iDlist.Load",CONTAINER_ERROR_NOMEMORY);
    	return NULL;
    }
    result = iDlist.Create(L.ElementSize);
    if (result == NULL) {
    	free(buf);
    	return NULL;
    }
    result->Flags = L.Flags;
    for (i=0; i < L.count; i++) {
    	int r = (int)loadFn(buf,arg,stream);
    	if (r <= 0) {
    		iError.RaiseError("iDlist.Load",r);
    		Finalize(result);
    		result=NULL;
    		break;
    	}
    	newl = new_dlink(result,buf,"iDlist.Load");
    	if (newl == NULL) {
    		Finalize(result);
    		result=NULL;
    		break;
    	}
    	if (i ==  0) {
    		result->First = newl;
    	}
    	else {
    		result->Last->Next = newl;
    		newl->Previous = result->Last;
    	}
    	result->Last = newl;
    	result->count++;
    }
    free(buf);
    return result;
}
static DestructorFunction SetDestructor(Dlist *cb,DestructorFunction fn)
{
    DestructorFunction oldfn;
    if (cb == NULL)
    	return NULL;
    oldfn = cb->DestructorFn;
    if (fn)
    	cb->DestructorFn = fn;
    return oldfn;
}
static ContainerMemoryManager *GetAllocator(Dlist *l)
{
    if (l == NULL)
        return NULL;
    return l->Allocator;
}


DlistInterface iDlist = {
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
    deleteIterator,
    Save,
    Load,
    GetElementSize,
/* Sequential container */
    Add,
    GetElement,
    PushFront,
    PopFront,
    InsertAt,
    EraseAt,
    ReplaceAt,
    IndexOf,
/* Dlist specific part */
    PushBack,
    PopBack,
    Splice,
    Sort,
    Reverse,
    GetRange,
    Append,
    SetCompareFunction,
    UseHeap,
    AddRange,
    Create,
    CreateWithAllocator,
    Init,
    CopyElement,
    InsertIn,
    SetDestructor,
    InitializeWith,
    GetAllocator,
};

