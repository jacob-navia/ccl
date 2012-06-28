#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif
#include "containers.h"
#include "ccl_internal.h"

static const guid DlistGuid = {0xac2525ff, 0x2e2a, 0x4540,
{0xae,0x70,0xc4,0x7a,0x2,0xf7,0xa,0xed}
};

#define CONTAINER_READONLY	1
#define LIST_HASPOINTER	2
#define CHUNK_SIZE	1000
static int Add_nd(Dlist *l,const void *elem);
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
static DlistElement *new_dlink(Dlist *l,const void *data,const char *fname)
{
    DlistElement *result;

    if (l->Heap == NULL) {
    	result = l->Allocator->malloc(sizeof(*result)+l->ElementSize);
    }
    else result = iHeap.NewObject(l->Heap);
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

static Dlist *InitWithAllocator(Dlist *result,size_t elementsize,const  ContainerAllocator *allocator)
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
    result->Allocator = allocator;
    return result;
}

static Dlist *Init(Dlist *result,size_t elementsize)
{
    return InitWithAllocator(result,elementsize,CurrentAllocator);
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
static Dlist *CreateWithAllocator(size_t elementsize,const ContainerAllocator *allocator)
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
    r = InitWithAllocator(result,elementsize,allocator);
    if (r == NULL)
    	allocator->free(result);
    return r;
}

static Dlist *Create(size_t elementsize)
{
    return CreateWithAllocator(elementsize,CurrentAllocator);
}

static Dlist *InitializeWith(size_t elementSize,size_t n,const void *Data)
{
        Dlist *result = Create(elementSize);
        size_t i;
        const char *pData = Data;
        if (result == NULL)
                return result;
        for (i=0; i<n; i++) {
                Add_nd(result,pData);
                pData += elementSize;
        }
        return result;
}


static int UseHeap(Dlist *L,const ContainerAllocator *m)
{
    if (L->Heap || L->count) {
    	L->RaiseError("iDlist.UseHeap",CONTAINER_ERROR_NOT_EMPTY);
    	return 0;
    }
    if (m == NULL)
    	m = L->Allocator;
    L->Heap = iHeap.Create(L->ElementSize+sizeof(DlistElement), m);
    if (L->Heap == NULL) {
    	L->RaiseError("iDlist.UseHeap",CONTAINER_ERROR_NOMEMORY);
    	return CONTAINER_ERROR_NOMEMORY;
    }
    return 1;

}

/* This function was proposed by Ben Pfaff in c.l.c if I remember correctly */
static Dlist *Splice ( Dlist *list, void *ppos, Dlist *toInsert, int dir )
{
    DlistElement *pos = ppos;
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
    	DlistElement *rvp = l->First,*tmp;

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
static int Add_nd(Dlist *l,const void *elem)
{
        DlistElement *newl = new_dlink(l,elem,"iList.Add");
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

static int Add(Dlist *l,const void *elem)
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

static int AddRange(Dlist * AL,size_t n, const void *data)
{
        const unsigned char *p;

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
    unsigned result;

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
static unsigned GetFlags(const Dlist *l)
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
static size_t Size(const Dlist *l)
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
static Dlist *Copy(const Dlist *l)
{
    Dlist *result;
    DlistElement *rvp,*newRvp,*last;

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
    		l->RaiseError("iDlist.Copy",CONTAINER_ERROR_NOMEMORY);
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

    if (l == NULL) return CONTAINER_ERROR_BADARG;
    
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
static void * GetElement(const Dlist *l,size_t position)
{
    DlistElement *rvp;


    if (l == NULL) {
    	iError.NullPtrError("iDlist.GetElement");
    	return 0;
    }
    if (position >= (signed)l->count) {
    	l->RaiseError("GetElement",CONTAINER_ERROR_INDEX,l,position);
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

static int ReplaceAt(Dlist *l,size_t position,const void *data)
{
    DlistElement *rvp;

    if (l == NULL || data == NULL) {
    	if (l)
    		l->RaiseError("iDlist.ReplaceAt",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.ReplaceAt");
    	return CONTAINER_ERROR_BADARG;
    }
    /* Error checking */
    if (position >= l->count) {
    	l->RaiseError("iDlist.ReplaceAt",CONTAINER_ERROR_INDEX,l,position);
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
    DlistElement *rvp;

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
static int Equal(const Dlist *l1,const Dlist *l2)
{
    DlistElement *link1,*link2;
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
static int PushFront(Dlist *l,const void *pdata)
{
    DlistElement *rvp;

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

static int PushBack(Dlist *l,const void *pdata)
{
    DlistElement *rvp;

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
    DlistElement *le;

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
    DlistElement *le;

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

static int InsertAt(Dlist *l,size_t pos,const void *pdata)
{
    DlistElement *elem;

    if (l == NULL || pdata == NULL) {
    	if (l)
    		l->RaiseError("iDlist.InsertAt",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.InsertAt");
    	return CONTAINER_ERROR_BADARG;
    }
    if (pos > l->count) {
    	l->RaiseError("iDlist.InsertAt",CONTAINER_ERROR_INDEX,l,pos);
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
    	DlistElement *rvp = l->First;
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
    DlistElement *le,*nle;

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
    	l->RaiseError("iDlist.InsertIn",CONTAINER_ERROR_INDEX,l,idx);
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
    DlistElement *rvp=NULL,*last;
    void *removed=NULL;

    if (l == NULL) return iError.NullPtrError("iDlist.InsertIn");
    
    if (position >= l->count) {
    	l->RaiseError("iDlist.EraseAt",CONTAINER_ERROR_INDEX,l,position);
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
    	iHeap.FreeObject(l->Heap,rvp);
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
    DlistElement *tmp,*rvp;

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
static int IndexOf(const Dlist *l,const void *ElementToFind, void * args,size_t *result)
{
    DlistElement *rvp;
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

static int EraseInternal(Dlist *l,const void *elem,int all)
{
    int r,result = CONTAINER_ERROR_NOTFOUND;
    CompareFunction fn;
    CompareInfo ci;
    DlistElement *rvp,*previous;
    size_t position;

    if (l == NULL || elem == NULL) {
    	if (l)
    		l->RaiseError("iDlist.Erase",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.Erase");
    	return CONTAINER_ERROR_BADARG;
    }
    position = 0;
    rvp = l->First;
    previous = NULL;
    fn = l->Compare;
    ci.ContainerLeft = l;
    ci.ContainerRight = NULL;
    ci.ExtraArgs = NULL;
    while (rvp) {
        r = fn(&rvp->Data,elem,&ci);
        if (r == 0) {
            if (l->Flags & CONTAINER_HAS_OBSERVER)
                iObserver.Notify(l,CCL_ERASE_AT,rvp,(void *)position);

            if (position == 0) {
                if (l->count == 1) {
                    l->First = l->Last = NULL;
                }
                else {
                    l->First = l->First->Next;
                    l->First->Previous = NULL;
                }
            }
            else if (position == l->count - 1) {
                previous->Next = NULL;
                l->Last = previous;
            }
            else {
                previous->Next = rvp->Next;
                rvp->Next->Previous = previous;
            }

            if (l->DestructorFn)
                l->DestructorFn(&rvp->Data);

            if (l->Heap)
                iHeap.FreeObject(l->Heap,rvp);
            else {
                l->Allocator->free(rvp);
            }
            l->count--;
            l->timestamp++;
            if (all == 0) return 1;
            result = 1;
        }
        previous = rvp;
        rvp = rvp->Next;
        position++;
    }
    return result;
}

static int Erase(Dlist *l,const void *elem)
{
    return EraseInternal(l,elem,0);
}
static int EraseAll(Dlist *l,const void *elem)
{
    return EraseInternal(l,elem,1);
}


/*------------------------------------------------------------------------
 Procedure:     Contains ID:1
 Purpose:       Determines if the given data is in the container
 Input:         The Dlist and the data to be searched
 Output:        Returns 1 (true) if the data is in there, false
 otherwise
 Errors:        The same as the function IndexOf
 ------------------------------------------------------------------------*/
static int Contains(const Dlist *l,const void *data)
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

static int CopyElement(const Dlist *l,size_t position,void *outBuffer)
{
    DlistElement *rvp;

    if (l == NULL || outBuffer == NULL) {
    	if (l)
    		l->RaiseError("iDlist.CopyElement",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.CopyElement");
    	return CONTAINER_ERROR_BADARG;
    }
    if (position >= l->count) {
    	l->RaiseError("iDlist.CopyElement",CONTAINER_ERROR_INDEX,l,position);
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
    DlistElement *Elem1 = *(DlistElement **)elem1;
    DlistElement *Elem2 = *(DlistElement **)elem2;
    Dlist *l = (Dlist *)ExtraArgs->ContainerLeft;
    CompareFunction fn = l->Compare;
    return fn(Elem1->Data,Elem2->Data,ExtraArgs);
}

static int Sort(Dlist *l)
{
    DlistElement **tab;
    size_t i;
    DlistElement *rvp;
    CompareInfo ci;

    if (l == NULL) return iError.NullPtrError("iDlist.Sort");
    
    if (l->Flags&CONTAINER_READONLY) {
    	l->RaiseError("iDlist.Sort",CONTAINER_ERROR_READONLY);
    	return CONTAINER_ERROR_READONLY;
    }
    if (l->count < 2)
    	return 1;
    tab = l->Allocator->malloc(l->count * sizeof(DlistElement *));
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
    qsortEx(tab,l->count,sizeof(DlistElement *),dlcompar,&ci);
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
    DlistElement *le;

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
    	return iError.RaiseError;
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
    DlistElement *rvp;

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

static void doinit(struct DListIterator *result,Dlist *L)
{
    result->it.GetNext = GetNext;
    result->it.GetPrevious = GetPrevious;
    result->it.GetFirst = GetFirst;
    result->it.GetCurrent = GetCurrent;
    result->it.Seek = Seek;
	result->it.Replace = ReplaceWithIterator;
    result->L = L;
    result->timestamp = L->timestamp;
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
    doinit(result,L);
    return &result->it;
}

static int InitIterator(Dlist *L,void *buf)
{
    struct DListIterator *result;
    
    if (L == NULL) {
    	iError.NullPtrError("iDlist.NewIterator");
    	return CONTAINER_ERROR_BADARG;
    }
    result = buf;
    doinit(result,L);
    return 1;
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


static size_t GetElementSize(const Dlist *d) 
{ 
    if (d == NULL) {
    	iError.NullPtrError("iDlist.GetElementSize");
    	return 0;
    }

    return d->ElementSize;
}
static size_t Sizeof(const Dlist *dl)
{
    if (dl == NULL) {
    	return sizeof(Dlist);
    }
    return sizeof(Dlist) + dl->count * (sizeof(DlistElement)+dl->ElementSize);
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
static int DefaultSaveFunction(const void *element,void *arg, FILE *Outfile)
{
    const unsigned char *str = element;
    size_t len = *(size_t *)arg;

    return len == fwrite(str,1,len,Outfile);
}

static int Save(const Dlist *L,FILE *stream, SaveFunction saveFn,void *arg)
{
    size_t i;
    DlistElement *rvp;
    size_t elmsiz;

    if (stream == NULL || L == NULL ) {
    	if (L)
    		L->RaiseError("iDlist.Save",CONTAINER_ERROR_BADARG);
    	else
    		iError.NullPtrError("iDlist.Save");
    	return CONTAINER_ERROR_BADARG;
    }
    if (saveFn == NULL) {
    	saveFn = DefaultSaveFunction;
        /* Copy element size to preserve const semantics */
        elmsiz = L->ElementSize;
    	arg = &elmsiz;
    }
    if (fwrite(&DlistGuid,sizeof(guid),1,stream) == 0)
    	return EOF;
    if (fwrite(L,1,sizeof(Dlist),stream) == 0)
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

static int DefaultLoadFunction(void *element,void *arg, FILE *Infile)
{
    size_t len = *(size_t *)arg;

    return len == fread(element,1,len,Infile);
}

static Dlist *Load(FILE *stream, ReadFunction loadFn,void *arg)
{
    size_t i;
    Dlist *result,L;
    DlistElement *newl;
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
    if (fread(&Guid,sizeof(guid),1,stream) == 0) {
    	iError.RaiseError("iDlist.Load",CONTAINER_ERROR_FILE_READ);
    	return NULL;
    }
    if (memcmp(&Guid,&DlistGuid,sizeof(guid))) {
    	iError.RaiseError("iDlist.Load",CONTAINER_ERROR_WRONGFILE);
    	return NULL;
    }
    if (fread(&L,1,sizeof(Dlist),stream) == 0) {
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
static const ContainerAllocator *GetAllocator(const Dlist *l)
{
    if (l == NULL)
        return NULL;
    return l->Allocator;
}

static void *Back(const Dlist *l)
{
    if (l == NULL) {
        iError.NullPtrError("Back");
        return NULL;
    }
    if (0 == l->count) {
        return NULL;
    }
    if (l->Flags & CONTAINER_READONLY) {
        l->RaiseError("iList.Back",CONTAINER_ERROR_READONLY);
        return NULL;
    }
    return l->Last->Data;
}
static void *Front(const Dlist *l)
{
    if (l == NULL) {
        iError.NullPtrError("Front");
        return NULL;
    }
    if (0 == l->count) {
        return NULL;
    }
    if (l->Flags & CONTAINER_READONLY) {
        l->RaiseError("iList.Front",CONTAINER_ERROR_READONLY);
        return NULL;
    }
    return l->First->Data;
}

static size_t SizeofIterator(const Dlist *l)
{
	return sizeof(struct DListIterator);
}

static int RemoveRange(Dlist *l,size_t start, size_t end)
{
    DlistElement *rvp,*rvpS,*rvpE;
    size_t position=0,tmp;

    if (l == NULL) {
        iError.RaiseError("RemoveRange",CONTAINER_ERROR_BADARG);
        return CONTAINER_ERROR_BADARG;
    }
    if (l->Flags&CONTAINER_READONLY) {
        l->RaiseError("RemoveRange",CONTAINER_ERROR_READONLY);
        return CONTAINER_ERROR_READONLY;
    }
    rvp = l->First;
    if (start >= l->count)
        return 0;
    if (end >= l->count)
        end = l->count;
    if (start == end) return 0;
    if (end < start) {
        tmp = end;
        end = start;
        start = tmp;
    }
    while (rvp && position != start) {
        rvp = rvp->Next;
        position++;
    }
    if (rvp == NULL) {
        l->RaiseError("RemoveRange",CONTAINER_INTERNAL_ERROR);
        return CONTAINER_INTERNAL_ERROR;
    }
    rvpS = rvp->Previous;
    while (rvp && position < end) {
        rvp = rvp->Next;
        position++;
    }
    if (rvp == NULL) {
        iError.RaiseError("RemoveRange",CONTAINER_ASSERTION_FAILED);
        return CONTAINER_ASSERTION_FAILED;
    }
    rvpE = rvp->Previous;
    if (rvpS) {
        if (rvpE) {
            rvpS->Next = rvpE;
            rvpE->Previous = rvpS;
        }
        else rvpS->Next = NULL;
    }
    if (start == 0) {
        l->First = rvpE;
        if (rvpE)
            rvpE->Previous = NULL;
    }
    if (end == l->count-1) {
        l->Last = rvpE;
        if (rvpE)
            rvpE->Next = NULL;
    }
    l->count -= (end-start-1);
    l->timestamp++;
    return 1;
}


static int Select(Dlist *src,const Mask *m)
{
    size_t i,offset=0;
    DlistElement *dst,*s,*removed;

    if (src == NULL || m == NULL) {
        return iError.NullPtrError("iDlist.Select");
    }
    if (src->Flags & CONTAINER_READONLY) {
        iError.RaiseError("Select",CONTAINER_ERROR_READONLY);
        return CONTAINER_ERROR_READONLY;
    }
    if (m->length != src->count) {
        iError.RaiseError("Select",CONTAINER_ERROR_BADMASK,src,m);
        return CONTAINER_ERROR_BADMASK;
    }
    if (src->count == 0) return 0;
    i=0;
    dst = src->First;
    while (i < m->length) {
        if (m->data[i]) break;
        if (src->DestructorFn)
            src->DestructorFn(dst->Data);
        removed = dst;
        dst = dst->Next;
        if (src->Heap) {
                iHeap.FreeObject(src->Heap, removed);
        } else
                src->Allocator->free(removed);
        i++;
    }
    if (i >= m->length) {
        // The mask was composed of only zeroes
        src->First = src->Last = NULL;
        src->count = 0;
        src->timestamp = 0;
        return 1;
    }
    src->First = dst;
    dst->Previous = NULL;
    i++;
    offset++;
    s = dst->Next;
    for (; i<m->length;i++) {
        if (m->data[i]) {
            dst->Next = s;
            s->Previous = dst;
            offset++;
            dst = s;
            s = s->Next;
        }
        else {
            if (src->DestructorFn) src->DestructorFn(s->Data);
            removed = s;
            s = s->Next;
            if (src->Heap) iHeap.FreeObject(src->Heap,removed);
            else src->Allocator->free(removed);
        }
    }
    dst->Next = NULL;
    src->Last = dst;
    src->count = offset;
    return 1;
}

static Dlist *SelectCopy(const Dlist *src,const Mask *m)
{
    Dlist *result;
    DlistElement *rvp;
    size_t i;
    int r;

    if (src == NULL || m == NULL) {
        iError.NullPtrError("iDlist.SelectCopy");
        return NULL;
    }
    if (m->length != src->count) {
        iError.RaiseError("iDlist.SelectCopy",CONTAINER_ERROR_BADMASK,src,m);
        return NULL;
    }
    result = Create(src->ElementSize);
    if (result == NULL) return NULL;
    rvp = src->First;
    for (i=0; i<m->length;i++) {
        if (m->data[i]) {
            r = Add_nd(result,rvp->Data);
            if (r < 0) {
                Finalize(result);
                return NULL;
            }
        }
        rvp = rvp->Next;
    }
    return result;
}


static DlistElement *FirstElement(Dlist *l)
{
	if (l == NULL) {
		iError.NullPtrError("FirstElement");
		return NULL;
	}
	if (l->Flags&CONTAINER_READONLY) {
		iError.RaiseError("FirstElement",CONTAINER_ERROR_READONLY);
		return NULL;
	}
	return l->First;
}

static DlistElement *LastElement(Dlist *l)
{
	if (l == NULL) {
		iError.NullPtrError("FirstElement");
		return NULL;
	}
	if (l->Flags&CONTAINER_READONLY) {
		iError.RaiseError("FirstElement",CONTAINER_ERROR_READONLY);
		return NULL;
	}
	return l->Last;
}

static DlistElement *NextElement(DlistElement *le)
{
	if (le == NULL) return NULL;
	return le->Next;
}

static DlistElement *PreviousElement(DlistElement *le)
{
	if (le == NULL) return NULL;
	return le->Previous;
}
static void *ElementData(DlistElement *le)
{
	if (le == NULL) return NULL;
	return le->Data;
}

static void *AdvanceInternal(DlistElement **ple,int goback)
{
	DlistElement *le;
	void *result;

	if (ple == NULL)
		return NULL;
	le = *ple;
	if (le == NULL) return NULL;
	result = le->Data;
	if (goback) le = le->Previous;
	else le = le->Next;
	*ple = le;
	return result;
}

static void *MoveBack(DlistElement **ple)
{
	return AdvanceInternal(ple,1);
}
static void *Advance(DlistElement **ple)
{
	return AdvanceInternal(ple,0);
}

static DlistElement *Skip(DlistElement *le, size_t n)
{
    if (le == NULL) return NULL;
    while (le != NULL && n > 0) {
        le = le->Next;
        n--;
    }
    return le;
}


static int RotateLeft(Dlist * l, size_t n)
{
    DlistElement    *rvp, *oldStart, *last = NULL;
    if (l == NULL)
        return iError.NullPtrError("iDlist.RotateLeft");
    if (l->Flags & CONTAINER_READONLY) {
        iError.RaiseError("iDlist.RotateLeft",CONTAINER_ERROR_READONLY);
        return CONTAINER_ERROR_READONLY;
    }
    if (l->count < 2 || n == 0)
        return 0;
    n %= l->count;
    if (n == 0)
        return 0;
    rvp = l->First;
    oldStart = rvp;
    while (n > 0) {
        last = rvp;
        rvp = rvp->Next;
        n--;
    }
    l->First = rvp;
    l->Last->Next = oldStart;
    oldStart->Previous = l->Last;
    l->Last = rvp->Previous;
    l->Last->Next = NULL;
    rvp->Previous = NULL;
    return 1;
}

static int RotateRight(Dlist * l, size_t n)
{
    DlistElement    *rvp, *oldStart, *last = NULL;
    if (l == NULL)
        return iError.NullPtrError("iDlist.RotateRight");
    if (l->Flags & CONTAINER_READONLY) {
        iError.RaiseError("iDlist.RotateLeft",CONTAINER_ERROR_READONLY);
        return CONTAINER_ERROR_READONLY;
    }
    if (l->count < 2 || n == 0)
        return 0;
    n %= l->count;
    if (n == 0)
        return 0;
    rvp = l->First;
    oldStart = rvp;
    n = l->count - n;
    while (n > 0) {
        last = rvp;
        rvp = rvp->Next;
        n--;
    }
    l->First = rvp;
    l->Last->Next = oldStart;
    oldStart->Previous = l->Last;
    l->Last = rvp->Previous;
    l->Last->Next = NULL;
    rvp->Previous = NULL;
    return 1;
}

static Dlist *SplitAfter(Dlist *l, DlistElement *pt)
{
    DlistElement *pNext;
    Dlist *result;
    size_t count=0;

    if (pt == NULL || l == NULL) {
        iError.NullPtrError("iList.SplitAfter");
        return NULL;
    }
    if (l->Flags&CONTAINER_READONLY) {
        iError.RaiseError("iDlist.SplitAfter",CONTAINER_ERROR_READONLY);
        return NULL;
    }
    pNext = pt->Next;
    if (pNext == NULL) return NULL;
    result = CreateWithAllocator(l->ElementSize, l->Allocator);
    if (result == NULL) return NULL;
    result->First = pNext;
    while (pNext) {
        count++;
        if (pNext->Next == NULL) result->Last = pNext;
        pNext = pNext->Next;
    }
    result->count = count;
    pt->Next = NULL;
    l->Last = pt;
    l->count -= count;
    l->timestamp++;
    return result;
}

static int SetElementData(Dlist *l, DlistElement *le,void *data)
{
    if (l == NULL || le == NULL || data == NULL) {
        return iError.NullPtrError("iList.SetElementData");
    }
    memcpy(le->Data,data,l->ElementSize);
    l->timestamp++;
    return 1;
}

DlistInterface iDlist = {
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
    InitWithAllocator,
    CopyElement,
    InsertIn,
    SetDestructor,
    InitializeWith,
    GetAllocator,
    Back,
    Front,
    RemoveRange,
    RotateLeft,
    RotateRight,
    Select,
    SelectCopy,
    FirstElement,
    LastElement,
    NextElement,
    PreviousElement,
    ElementData,
    SetElementData,
    Advance,
    Skip,
    MoveBack,
    SplitAfter,
};

