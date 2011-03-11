#include "containers.h"

typedef struct _list_element {
    struct _list_element *Next;
    char Data[MINIMUM_ARRAY_INDEX];
} list_element;
struct tagHeapObject {
	HeapInterface *VTable;
	unsigned BlockCount;
	unsigned CurrentBlock;
	unsigned BlockIndex;
	char **Heap;
	size_t ElementSize;
	void *FreeList;
	ContainerMemoryManager *Allocator;
	size_t MemoryUsed;
};	
static ContainerMemoryManager DefaultMemoryManagerObject = { malloc,free,realloc,calloc};
ContainerMemoryManager *CurrentMemoryManager = &DefaultMemoryManagerObject;
ContainerMemoryManager *SetCurrentMemoryManager(ContainerMemoryManager *in)
{
	ContainerMemoryManager *c;
	if (in == NULL) 
		return CurrentMemoryManager;
	c = CurrentMemoryManager;
	CurrentMemoryManager = in;
	return c;
}
#ifndef CHUNK_SIZE 
#define CHUNK_SIZE 100
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
		siz = sizeof(list_element *)*CHUNK_SIZE;
		l->Heap = l->Allocator->malloc(siz);
		if (l->Heap == NULL) {
			return NULL;
		}
		l->MemoryUsed += siz;
		l->BlockCount = CHUNK_SIZE;
		l->CurrentBlock=0;
		l->BlockIndex = 0;
		memset(l->Heap,0,siz);
	}
	if (l->FreeList) {
		list_element *le = l->FreeList;
		l->FreeList = le->Next;
		return le;
	}
	if (l->BlockIndex == CHUNK_SIZE) {
		/* The current block is full */
		l->CurrentBlock++;
		if (l->CurrentBlock == l->BlockCount) {
			/* The array of block pointers is full. Allocate CHUNK_SIZE blocks more */
			siz = (l->BlockCount+CHUNK_SIZE)*sizeof(list_element *);
			result = l->Allocator->realloc(l->Heap,siz);
			if (result == NULL) {
				return NULL;
			}
			l->MemoryUsed+= siz;
			l->Heap = (char **)result;
			/* Position pointer at the start of the new area */
			result += l->BlockCount*sizeof(list_element *);
			/* Zero the new pointers */
			memset(result,0,siz);
			l->BlockCount += CHUNK_SIZE;
		}
	}
	if (l->Heap[l->CurrentBlock] == NULL) {
		siz = CHUNK_SIZE*(sizeof(void *)+l->ElementSize);
		result = l->Allocator->malloc(siz);
		if (result == NULL) {
			return NULL;
		}
		memset(result,0,siz);
		l->Heap[l->CurrentBlock] = result;
		l->BlockIndex = 0;
	}
	l->BlockIndex++;
	result = l->Heap[l->CurrentBlock];
	result += l->ElementSize*l->BlockIndex;
	return result;
}

static void AddToFreeList(ContainerHeap *heap,void *element)
{
	list_element *le = (list_element *)element;
	le->Next = heap->FreeList;
	heap->FreeList = le;
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
#ifdef NO_GC	
	size_t i;
	ContainerMemoryManager *m = l->Allocator;
	
	for (i=0;i<l->BlockCount;i++) {
		m->free(l->Heap[i]);
	}
	m->free(l->Heap);
#endif
}

static void Clear(ContainerHeap *heap)
{
	heap->FreeList = NULL;
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

static ContainerHeap *InitHeap(void *pHeap,size_t ElementSize,ContainerMemoryManager *m)
{
	ContainerHeap *heap = pHeap;
	memset(heap,0,sizeof(*heap));
	heap->VTable = &iHeap;
	heap->ElementSize = ElementSize;
	if (m == NULL)
		m = CurrentMemoryManager;
	heap->Allocator = m;
	return heap;
}

static ContainerHeap *newHeap(size_t ElementSize,ContainerMemoryManager *m)
{
	ContainerHeap *result = m->malloc(sizeof(ContainerHeap));
	if (result == NULL)
		return NULL;
	InitHeap(result,ElementSize,m);
	return result;
}

struct HeapIterator {
	Iterator it;
	ContainerHeap *Heap;
	size_t BlockNumber;
	size_t BlockPosition;
	size_t timestamp;
	unsigned long Flags;

} ;

static void *GetFirst(Iterator *it)
{
	struct HeapIterator *hi = (struct HeapIterator *)it;
	ContainerHeap *heap = hi->Heap;
	char *result;

	hi->BlockNumber = hi->BlockPosition = 0;
	if (heap->BlockCount == 0)
		return NULL;
	result = heap->Heap[0];
	return result;
}


static void *GetNext(Iterator *it)
{
	struct HeapIterator *hi = (struct HeapIterator *)it;
	ContainerHeap *heap = hi->Heap;
	char *result;
	if (hi->BlockNumber == (heap->BlockCount-1)) {
		/* In the last block we should not got beyond the
		   last used element, the block can be half full.
		*/
		if (hi->BlockPosition >= heap->BlockIndex)
			return NULL;
		result = heap->Heap[hi->BlockNumber];
		result += (hi->BlockPosition*heap->ElementSize);
		hi->BlockPosition++;
		return result;
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
	result = heap->Heap[hi->BlockNumber];
	result += (hi->BlockPosition*heap->ElementSize);
	hi->BlockPosition++;
	return result;
}

static void *GetPrevious(Iterator *it)
{
	struct HeapIterator *hi = (struct HeapIterator *)it;
	ContainerHeap *heap = hi->Heap;
	char *result;
	if (hi->BlockPosition == 0) {
		/* Go to the last element of the previous block		*/
		if (hi->BlockNumber == 0)
			return NULL;
		hi->BlockNumber--;
		hi->BlockPosition = CHUNK_SIZE -1;
		result = heap->Heap[hi->BlockNumber];
		result += (hi->BlockPosition*heap->ElementSize);
		return result;
	}
	hi->BlockPosition--;
	result = heap->Heap[hi->BlockNumber];
	result += (hi->BlockPosition*heap->ElementSize);
	return result;
}

#ifdef notused
static void *GetLast(Iterator *it)
{
	struct HeapIterator *hi = (struct HeapIterator *)it;
	ContainerHeap *heap = hi->Heap;
	char *result;
	if (heap->BlockCount == 0)
		return NULL;
	hi->BlockNumber = heap->BlockCount-1;
	hi->BlockPosition = heap->BlockIndex-1;
	result = heap->Heap[heap->BlockCount-1];
	if (heap->BlockIndex)
		result += (heap->BlockIndex-1) *heap->ElementSize;
	return result;
}
#endif

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

static Iterator *newIterator(ContainerHeap *heap)
{
	struct HeapIterator *result;

	if (heap == NULL) {
		iError.RaiseError("iHeap.newIterator",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	result = heap->Allocator->malloc(sizeof(struct HeapIterator));
	if (result == NULL)
		return NULL;
	memset(result,0,sizeof(result));
	result->Heap = heap;
	result->timestamp = heap->CurrentBlock+heap->BlockIndex;
	result->it.GetFirst = GetFirst;
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetPrevious;
	result->it.GetCurrent = GetCurrent;
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
	AddToFreeList,
	Clear,
	DestroyHeap,
	InitHeap,
	GetHeapSize,
	newIterator,
	deleteIterator
};
