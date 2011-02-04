#include "containers.h"

struct _StreamBuffer {
	size_t Size;
	size_t Cursor;
	unsigned Flags;
	ContainerMemoryManager *Allocator;
	char *Data;
} ;

#define SB_READ_FLAG   1
#define SB_WRITE_FLAG  2
#define SB_APPEND_FLAG 4
#define SB_FIXED       8

#define FCLOSE_DESTROYS 8

static StreamBuffer *CreateWithAllocator(size_t size,ContainerMemoryManager *Allocator)
{
	StreamBuffer *result;
	
	if (size == 0) 
		size = 1024;
	result = Allocator->malloc(sizeof(StreamBuffer));
	if (result == NULL) {
		iError.RaiseError("iStreamBuffer.Create",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memset(result,0,sizeof(StreamBuffer));
	result->Allocator = Allocator;
	result->Data = Allocator->malloc(size);
	if (result->Data == NULL) {
		Allocator->free(result);
		return NULL;
	}
	result->Size = size;
	memset(result->Data,0,size);
	return result;
}

static StreamBuffer *Create(size_t size)
{
	return CreateWithAllocator(size,CurrentMemoryManager);
}

static int resizeBuffer(StreamBuffer *b,size_t chunkSize)
{
	char *p;
	size_t newSiz;
	
	if (chunkSize < b->Size/2)
		newSiz = b->Size/2;
	else newSiz = chunkSize;
	p = b->Allocator->realloc(b->Data,b->Size+newSiz);
	if (p == NULL)
		return CONTAINER_ERROR_NOMEMORY;
	b->Data = p;
	b->Size += newSiz;
	return 1;
}

static size_t Write(StreamBuffer *b,void *data, size_t siz)
{
	if (b == NULL) {
		iError.RaiseError("iStreamBuffer.Write",CONTAINER_ERROR_BADARG);
		return 0;
	}
	if (b->Cursor + siz > b->Size) {
		int r = resizeBuffer(b,siz);
		if (r < 0)
			return 0;
	}
	memcpy(b->Data+b->Cursor,data,siz);
	b->Cursor += siz;
	return siz;
}

static size_t Read(StreamBuffer *b, void *data, size_t siz)
{
	size_t len;
	if (b == NULL || data == NULL || siz == 0) {
		iError.RaiseError("iStreamBuffer.Add",CONTAINER_ERROR_BADARG);
		return 0;
	}
	if (b->Cursor >= b->Size)
		return 0;
	len = siz;
	if (b->Size - b->Cursor < len)
		len = b->Size - b->Cursor;
	memcpy(data,b->Data+b->Cursor,len);
	b->Cursor += len;
	return len;
}

static int SetPosition(StreamBuffer *b,size_t pos)
{
	if (b == NULL) {
		iError.RaiseError("iStreamBuffer.SetPosition",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (pos > b->Size)
		pos = b->Size;
	b->Cursor = pos;
	return 1;
}

static size_t GetPosition(StreamBuffer *b)
{
	if (b == NULL)
		return 0;
	return b->Cursor;
}

static size_t StreamBufferSize(StreamBuffer *b)
{
	if (b == NULL)
		return sizeof(StreamBuffer);
	return b->Size;
}

static int Clear(StreamBuffer *b)
{
	if (b == NULL) {
		iError.RaiseError("iStreamBuffer.Clear",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	memset(b->Data,0,b->Size);
	b->Cursor = 0;
	return 1;
}

static int Finalize(StreamBuffer *b)
{
	if (b == NULL) {
		iError.RaiseError("iStreamBuffer.Finalize",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	b->Allocator->free(b->Data);
	b->Allocator->free(b);
	return 1;
}

static char *GetData(StreamBuffer *b)
{
	if (b == NULL) {
		iError.RaiseError("iStreamBuffer.Finalize",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	return b->Data;
}	
StreamBufferInterface iStreamBuffer = {
Create,
CreateWithAllocator,
Read,
Write,
SetPosition,
GetPosition,
	GetData,
StreamBufferSize,
Clear,
Finalize,
};
/* --------------------------------------------------------------------------
                               Circular buffers
   ------------------------------------------------------------------------- */
struct _CircularBuffer {
	size_t maxCount;
	size_t ElementSize;
	size_t head;
	size_t tail;
	ContainerMemoryManager *Allocator;
	DestructorFunction DestructorFn;
	unsigned char *data;
};


static DestructorFunction SetDestructor(CircularBuffer *cb,DestructorFunction fn)
{
	DestructorFunction oldfn;
	if (cb == NULL)
		return NULL;
	oldfn = cb->DestructorFn;
	if (fn)
		cb->DestructorFn = fn;
	return oldfn;
}
	
static size_t Size(CircularBuffer *cb)
{
	if (cb == NULL)
		return sizeof(CircularBuffer);
	return cb->head - cb->tail;
}

static int Add( CircularBuffer * b, void *data_element)
{
    unsigned char *ring_data = NULL;
	int result = 1;

    if (b == NULL || data_element == NULL) {
        iError.RaiseError("iCircularBuffer.Add",CONTAINER_ERROR_BADARG);
        return CONTAINER_ERROR_BADARG;
    }
    if (b->maxCount == (b->head - b->tail)) {
        b->head = 0;
		result = 0;
    }
    ring_data = b->data + ((b->head % b->maxCount) * b->ElementSize);
    memcpy(ring_data,data_element,b->ElementSize);
    b->head++;
    return result;
}

static int PopFront(CircularBuffer *b,void *result)
{
	void *data;
	
    if (b == NULL) {
        iError.RaiseError("iCircularBuffer.PopFront",CONTAINER_ERROR_BADARG);
        return CONTAINER_ERROR_BADARG;
    }
	if (b->head == b->tail)
		return 0;
    data = &(b->data[(b->tail % b->maxCount) * b->ElementSize]);
	b->tail++;
	if (result)
		memcpy(result,data,b->ElementSize);
	return 1;
}

static int PeekFront(CircularBuffer *b,void *result)
{
	void *data;
	
    if (b == NULL || result == NULL) {
        iError.RaiseError("iCircularBuffer.PeekFront",CONTAINER_ERROR_BADARG);
        return CONTAINER_ERROR_BADARG;
    }
	if (b->head == b->tail)
		return 0;
    data = &(b->data[(b->tail % b->maxCount) * b->ElementSize]);
	memcpy(result,data,b->ElementSize);
	return 1;
}

static CircularBuffer *CreateCBWithAllocator(size_t sizElement,size_t sizeBuffer,ContainerMemoryManager *allocator)
{
	CircularBuffer *result;
	
	if (sizElement == 0 || sizeBuffer == 0 || allocator == NULL) {
		iError.RaiseError("iCircularBuffer.Create",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	result = allocator->malloc(sizeof(CircularBuffer));
	if (result == NULL) {
		iError.RaiseError("iCircularBuffer.Create",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memset(result,0,sizeof(CircularBuffer));
	result->maxCount = sizeBuffer;
	result->ElementSize = sizElement;
	result->Allocator = allocator;
	sizElement = sizElement*sizeBuffer; /* Here we should test for overflow */
	result->data = allocator->malloc(sizElement);
	if (result->data == NULL) {
		allocator->free(result);
		iError.RaiseError("iCircularBuffer.Create",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	return result;
}

static CircularBuffer *CreateCB(size_t sizElement,size_t sizeBuffer)
{
	return CreateCBWithAllocator(sizElement, sizeBuffer, CurrentMemoryManager);
}

static int CBClear(CircularBuffer *cb)
{
	unsigned char *p;
	size_t i;
	if (cb == NULL) {
		iError.RaiseError("iCircularBuffer.Clear",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	if (cb->head == cb->tail)
		return 0;
	if (cb->DestructorFn) {
		p = cb->data;
		for (i=cb->tail; i<cb->head;i++) {
			cb->DestructorFn(p);
			p += cb->ElementSize;
		}
	}
	memset(p,0,cb->ElementSize*(cb->head-cb->tail));
	cb->head = cb->tail = 0;
	return 1;
}

static int CBFinalize(CircularBuffer *cb)
{
	if (cb == NULL) {
		iError.RaiseError("iCircularBuffer.Finalize",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	CBClear(cb);
	cb->Allocator->free(cb->data);
	cb->Allocator->free(cb);
	return 1;
}

static size_t Sizeof(CircularBuffer *cb)
{
	size_t result = sizeof(CircularBuffer);
	if (cb == NULL)
		return result;
	result += (cb->head - cb->tail)*cb->ElementSize;
	return result;
}

CircularBufferInterface iCircularBuffer = {
	Size,
	Add,
	PopFront,
	PeekFront,
	CreateCBWithAllocator,
	CreateCB,
	CBClear,
	CBFinalize,
	Sizeof,
	SetDestructor,
};
