#include "containers.h"
#include "ccl_internal.h"

#define INVALID_POINTER_VALUE (void *)(~0)
struct tagHeapObject {
    HeapInterface *VTable;
    unsigned BlockCount;
    unsigned CurrentBlock;
    unsigned BlockIndex;
    char **Heap;
    size_t ElementSize;
    void *FreeList;
    const ContainerAllocator *Allocator;
    size_t MemoryUsed;
    unsigned timestamp;
};
#ifndef CHUNK_SIZE 
#define CHUNK_SIZE 1000
#endif
/*------------------------------------------------------------------------
 Procedure:     new_HeapObject ID:1
 Purpose:       Allocation of a new list element. If the element
                size is zero, we have an heterogenous
                list, and we allocate just a pointer to the data
                that is maintained by the user.
                Note that we allocate the size of a list element
                plus the size of the data in a single
                block. This block should be passed to the FREE
                function.

 Input:         The list where the new element should be added and a
                pointer to the data that will be added (can be
                NULL).
 Output:        A pointer to the new list element (can be NULL)
 Errors:        If there is no memory returns NULL
------------------------------------------------------------------------*/ 
static void *newHeapObject(ContainerHeap *l)
{
    size_t siz;
    char *result;

    if (l->Heap == NULL) {
        /* Allocate an array of pointers that will hold the blocks
        of CHUNK_SIZE list items
        */
        l->Heap = l->Allocator->calloc(CHUNK_SIZE,sizeof(ListElement *));
        if (l->Heap == NULL) {
            return NULL;
        }
        l->MemoryUsed += sizeof(ListElement *)*CHUNK_SIZE;
        l->BlockCount = CHUNK_SIZE;
        l->CurrentBlock=0;
        l->BlockIndex = 0;
    }
    if (l->FreeList) {
        ListElement *le = l->FreeList;
        l->FreeList = le->Next;
        return le;
    }
    if (l->BlockIndex >= CHUNK_SIZE) {
        /* The current block is full */
        l->CurrentBlock++;
        if (l->CurrentBlock == l->BlockCount) {
            /* The array of block pointers is full. Allocate CHUNK_SIZE elements more */
            siz = (l->BlockCount+CHUNK_SIZE)*sizeof(ListElement *);
            result = l->Allocator->realloc(l->Heap,siz);
            if (result == NULL) {
                return NULL;
            }
            l->Heap = (char **)result;
            l->MemoryUsed += CHUNK_SIZE*sizeof(ListElement *);
            /* Position pointer at the start of the new area */
            result += l->BlockCount*sizeof(ListElement *);
            /* Zero the new pointers */
            siz = CHUNK_SIZE*sizeof(ListElement *);
            memset(result,0,siz);
            l->BlockCount += CHUNK_SIZE;
        }
    }
    if (l->Heap[l->CurrentBlock] == NULL) {
        result = l->Allocator->calloc(CHUNK_SIZE,sizeof(void *)+l->ElementSize);
        if (result == NULL) {
            return NULL;
        }
        l->Heap[l->CurrentBlock] = result;
        l->BlockIndex = 0;
	l->MemoryUsed += CHUNK_SIZE * (sizeof(void *) + l->ElementSize);
    }
    result = l->Heap[l->CurrentBlock];
    result += l->ElementSize * l->BlockIndex;
    l->BlockIndex++;
    l->timestamp++;
    return result;
}

#ifdef DEBUG_HEAP_FREELIST
static size_t FindBlock(ContainerHeap *heap,void *elem, size_t *idx)
{
    size_t i;
    intptr_t blockStart,blockEnd,e = (intptr_t)elem;

    for (i=0; i<=heap->BlockIndex;i++) {
        blockStart = (intptr_t) heap->Heap[i];
        blockEnd = blockStart + CHUNK_SIZE*sizeof(ListElement *);
        if (e >= blockStart && e < blockEnd) {
            if (((e-blockStart) % sizeof(ListElement)) == 0) {
                *idx = (e-blockStart)/sizeof(ListElement);
                return i;
            }
            else
                return 1+heap->BlockIndex;
        }
    }
    return i;
}
#endif

static int FreeObject(ContainerHeap *heap,void *element)
{
    ListElement *le = element;

    le->Next = INVALID_POINTER_VALUE;
#ifdef DEBUG_HEAP_FREELIST
    { size_t idx,blockNr;
        blockNr = FindBlock(heap,element,&idx);
        if (blockNr > heap->CurrentBlock) return -1;
    }
#endif
    memcpy(le->Data, &heap->FreeList,sizeof(ListElement *));
    heap->FreeList = le;
    heap->timestamp++;
    return 1;
}

static void Clear(ContainerHeap * heap)
{
    size_t i;

    for (i=0; i<heap->BlockCount;i++) heap->Allocator->free(heap->Heap[i]);
    heap->Allocator->free(heap->Heap);
    heap->BlockCount = 0;
    heap->CurrentBlock = 0;
    heap->BlockIndex = 0;
    heap->Heap = NULL;
    heap->MemoryUsed = 0;
    heap->timestamp=0;
}

/*------------------------------------------------------------------------
 Procedure:     DestroyListElements ID:1
 Purpose:       Reclaims all memory used by a list
 Input:         The list
 Output:        None
 Errors:        None
 ------------------------------------------------------------------------*/
static void DestroyHeap(ContainerHeap *l)
{
    Clear(l);
    l->Allocator->free(l);
}

static size_t GetHeapSize(ContainerHeap *heap)
{
    size_t result;
    
    if (heap->Heap == NULL)
        return 0;
    result = CHUNK_SIZE * (heap->CurrentBlock+1) * heap->ElementSize;
    result += heap->BlockIndex * (heap->ElementSize);
    result += (sizeof(void *) * heap->BlockCount);
    return result;
}

static ContainerHeap *InitHeap(void *pHeap,size_t ElementSize,const ContainerAllocator *m)
{
    ContainerHeap *heap = pHeap;
    memset(heap,0,sizeof(*heap));
    heap->VTable = &iHeap;
    if (ElementSize < 2*sizeof(ListElement *))
        ElementSize = 2*sizeof(ListElement *);
    heap->ElementSize = ElementSize;
    if (m == NULL)
        m = CurrentAllocator;
    heap->Allocator = m;
    return heap;
}

static  ContainerHeap *newHeap(size_t ElementSize,const ContainerAllocator *m)
{
    ContainerHeap *result = m->malloc(sizeof(ContainerHeap));
    if (result == NULL)
        return NULL;
    return InitHeap(result,ElementSize,m);
}

struct HeapIterator {
    Iterator it;
    ContainerHeap *Heap;
    size_t BlockNumber;
    size_t BlockPosition;
    size_t timestamp;
    unsigned long Flags;

} ;

static int SkipFreeForward(struct HeapIterator *it)
{
    size_t idx = it->BlockNumber * CHUNK_SIZE + it->BlockPosition;
    size_t start = idx;
    ListElement *le;
    char *p;

    p = it->Heap->Heap[it->BlockNumber];
    p += it->BlockPosition*it->Heap->ElementSize;
    le = (ListElement *)p;

    while (le->Next == INVALID_POINTER_VALUE) {
        idx++;
        it->BlockPosition++;
        if (it->BlockPosition >= CHUNK_SIZE) {
            it->BlockNumber++;
            if (it->BlockNumber >= it->Heap->BlockCount)
                return -1;
            p = it->Heap->Heap[it->BlockNumber];
            it->BlockPosition = 0;
        }
        else {
            /* Do not go beyond the last position in the last block */
            if (it->BlockNumber == it->Heap->BlockCount &&
                it->BlockPosition >= it->Heap->BlockIndex)
                return -1;
            p = it->Heap->Heap[it->BlockNumber];
            p += (it->BlockPosition)*it->Heap->ElementSize;
        }
        le = (ListElement *)p;
    }
    if (idx == start) return 0;
    return 1;
}

static int SkipFreeBackwards(struct HeapIterator *it)
{
    size_t idx = it->Heap->CurrentBlock * CHUNK_SIZE + it->Heap->BlockIndex;
    size_t start = idx;
    ListElement *le;
    char *p;

    p = it->Heap->Heap[it->BlockNumber];
    p += it->BlockPosition*it->Heap->ElementSize;
    le = (ListElement *)p;

    while (le->Next == INVALID_POINTER_VALUE) {
        idx--;
        if (it->Heap->BlockIndex > 0) {
            p -= it->Heap->ElementSize;
            it->BlockPosition--;
        }
        else {
            if (it->BlockNumber == 0)
                return -1;
            it->BlockNumber--;
            it->BlockPosition = CHUNK_SIZE-1;
            p = it->Heap->Heap[it->BlockNumber];
            p += (CHUNK_SIZE-1)*it->Heap->ElementSize;
        }
        le = (ListElement *)p;
    }
    if (idx == start) return 0;
    return 1;
}

static void *GetFirst(Iterator *it)
{
    struct HeapIterator *hi = (struct HeapIterator *)it;
    ContainerHeap *heap = hi->Heap;
    char *result;
    int r;

    hi->BlockNumber = hi->BlockPosition = 0;
    if (heap->BlockCount == 0)
        return NULL;
    r = SkipFreeForward(hi);
    if (r < 0) return NULL;
    result = heap->Heap[hi->BlockNumber];
    result += (hi->BlockPosition*heap->ElementSize);
    if (r == 0) hi->BlockPosition++;
    return result;
}


static void *GetNext(Iterator *it)
{
    struct HeapIterator *hi = (struct HeapIterator *)it;
    ContainerHeap *heap = hi->Heap;
    char *result;
    
    if (hi->BlockNumber == (heap->CurrentBlock)) {
        /* In the last block we should not got beyond the
           last used element, the block can be half full.
        */
        if (hi->BlockPosition >= heap->BlockIndex)
            return NULL;
    }
    /*
        We are in a block that is full. Check that the position
        is less than the size of the block.
        */
    if (hi->BlockPosition >= CHUNK_SIZE) {
        hi->BlockNumber++;
        hi->BlockPosition = 0;
        return GetNext(it);
    }
    if (SkipFreeForward(hi) < 0)
        return NULL;
    result = heap->Heap[hi->BlockNumber];
    result += (hi->BlockPosition*heap->ElementSize);
    hi->BlockPosition++;
    return result;
}

static size_t GetPosition(Iterator *it)
{
    struct HeapIterator *hi = (struct HeapIterator *)it;
    return hi->BlockNumber*CHUNK_SIZE + hi->BlockPosition;
}

static void *GetPrevious(Iterator *it)
{
    struct HeapIterator *hi = (struct HeapIterator *)it;
    ContainerHeap *heap = hi->Heap;
    char *result;
    int r;

    if (hi->BlockPosition == 0) {
        /* Go to the last element of the previous block        */
        if (hi->BlockNumber == 0)
            return NULL;
        hi->BlockNumber--;
        hi->BlockPosition = CHUNK_SIZE -1;
        result = heap->Heap[hi->BlockNumber];
        result += (hi->BlockPosition*heap->ElementSize);
        return result;
    }
    r = SkipFreeBackwards(hi);
    if (r < 0) 
        return NULL;
    if (r == 0) hi->BlockPosition--;
    result = heap->Heap[hi->BlockNumber];
    result += (hi->BlockPosition*heap->ElementSize);
    return result;
}

static void *GetLast(Iterator *it)
{
    struct HeapIterator *hi = (struct HeapIterator *)it;
    ContainerHeap *heap = hi->Heap;
    char *result;
    if (heap->BlockCount == 0)
        return NULL;
    hi->BlockNumber = heap->CurrentBlock;
    hi->BlockPosition = heap->BlockIndex-1;
    result = heap->Heap[heap->CurrentBlock];
    result += (hi->BlockPosition) *heap->ElementSize;
    return result;
}

static void *GetCurrent(Iterator *it)
{
    struct HeapIterator *hi = (struct HeapIterator *)it;
    ContainerHeap *heap = hi->Heap;
    char *result;
    if (heap->BlockCount == 0)
        return NULL;
    result = heap->Heap[hi->BlockNumber];
    result += (hi->BlockPosition) *heap->ElementSize;
    return result;
}

static Iterator *NewIterator(ContainerHeap *heap)
{
    struct HeapIterator *result;

    if (heap == NULL) {
        iError.RaiseError("iHeap.NewIterator",CONTAINER_ERROR_BADARG);
        return NULL;
    }
    result = heap->Allocator->calloc(1,sizeof(struct HeapIterator));
    if (result == NULL)
        return NULL;
    result->Heap = heap;
    result->timestamp = heap->CurrentBlock+heap->BlockIndex;
    result->it.GetFirst = GetFirst;
    result->it.GetNext = GetNext;
    result->it.GetPrevious = GetPrevious;
    result->it.GetCurrent = GetCurrent;
    result->it.GetLast = GetLast;
    result->it.GetPosition = GetPosition;
    return &result->it;
}

static int deleteIterator(Iterator *it)
{
    struct HeapIterator *hi = (struct HeapIterator *)it;
    ContainerHeap *heap = hi->Heap;
    heap->Allocator->free(it);
    return 1;
}

HeapInterface iHeap = {
    newHeap,
    newHeapObject,
    FreeObject,
    Clear,
    DestroyHeap,
    InitHeap,
    GetHeapSize,
    NewIterator,
    deleteIterator
};
