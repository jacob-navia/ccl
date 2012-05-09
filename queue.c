#include "containers.h"
/* -------------------------------------------------------------------
 *                           QUEUES
 * -------------------------------------------------------------------*/
typedef struct _Queue {
    QueueInterface *VTable;
    List *Items;
} _Queue;

static size_t QSize(Queue *Q)
{
	return iList.Size(Q->Items);
}

static size_t Sizeof(Queue *q)
{
    if (q == NULL) return sizeof(Queue);
    return sizeof(*q) + iList.Sizeof(q->Items);
}

static int Finalize(Queue *Q)
{
    const ContainerAllocator *allocator = iList.GetAllocator(Q->Items);
    iList.Finalize(Q->Items);
    allocator->free(Q);
    return 1;
}

static int QClear(Queue *Q)
{
    return iList.Clear(Q->Items);
}


static int Dequeue(Queue *Q,void *result)
{
    return iList.PopFront(Q->Items,result);
}

static int Enqueue(Queue *Q,void *newval)
{
    return iList.Add(Q->Items,newval);
}


static Queue *CreateWithAllocator(size_t ElementSize,ContainerAllocator *allocator)
{
    Queue *result = allocator->malloc(sizeof(Queue));

    if (result == NULL)
        return NULL;
    result->Items = iList.CreateWithAllocator(ElementSize,allocator);
    if (result->Items == NULL) {
        allocator->free(result);
        return NULL;
    }
    result->VTable = &iQueue;
    return result;
}

static Queue *Create(size_t ElementSize)
{
	return CreateWithAllocator(ElementSize,CurrentAllocator);
}
static int Front(Queue *Q,void *result)
{
	size_t idx;
	if (Q == NULL) {
		iError.RaiseError("iQueue.Front",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	idx = iList.Size(Q->Items);
	if (idx == 0)
		return 0;
	return iList.CopyElement(Q->Items,0,result);
}

static int Back(Queue *Q,void *result)
{
	size_t idx;
	if (Q == NULL) {
		iError.RaiseError("iQueue.Front",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	idx = iList.Size(Q->Items);
	if (idx == 0)
		return 0;
	return iList.CopyElement(Q->Items,idx-1,result);
}

static List *GetData(Queue *q)
{
	if (q == NULL) {
		iError.RaiseError("iQueue.GetData",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	return q->Items;
}


QueueInterface iQueue = {
    Create,
	CreateWithAllocator,
    QSize,
    Sizeof,
    Enqueue,
    Dequeue,
    QClear,
    Finalize,
	Front,
	Back,
	GetData,
};
