#ifndef __containers_h__
#define __containers_h__
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#ifdef _MSC_VER
/* Use the library provided stdint.h since microsoft doesn't
   provide one. Note that it is provided for 32 bits only */
#include "stdint.h"
#pragma warning(disable:4100)
#pragma warning(disable:4127)
#pragma warning(disable:4232)
#pragma warning(disable:4820)
#else
#include <stdint.h>
#endif
#include <limits.h>

/*-------------------------------------------------------------
   Default settings: NO_C99
---------------------------------------------------------------*/

#define NO_GC
#define NO_C99

#ifdef NO_C99
/* No stdbool */
typedef enum { FALSE,TRUE}  bool;
#else
/* Use C99 features */
#include <stdbool.h>
#endif

/* General container flags */
#define CONTAINER_READONLY	1
#define CONTAINER_HAS_OBSERVER	2
/************************************************************************** */
/*                                                                          */
/*                          ErrorHandling                                   */
/*                                                                          */
/************************************************************************** */
#define CONTAINER_ERROR_NOTFOUND        -2
#define CONTAINER_ERROR_INDEX           -3
#define CONTAINER_ERROR_READONLY        -4
#define CONTAINER_ERROR_FILEOPEN        -5
#define CONTAINER_ERROR_WRONGFILE       -6
#define CONTAINER_ERROR_NOTIMPLEMENTED  -7
#define CONTAINER_INTERNAL_ERROR        -8
#define CONTAINER_ERROR_OBJECT_CHANGED  -9
#define CONTAINER_ERROR_NOT_EMPTY       -10
#define CONTAINER_ERROR_FILE_READ       -11
#define CONTAINER_ERROR_FILE_WRITE      -12
#define CONTAINER_FULL                  -13
#define CONTAINER_ASSERTION_FAILED      -14
#define CONTAINER_ERROR_BADARG	        -15
#define CONTAINER_ERROR_NOMEMORY        -16
#define CONTAINER_ERROR_NOENT           -17
#define CONTAINER_ERROR_INCOMPATIBLE    -18
#define CONTAINER_ERROR_BADPOINTER      -19
#define CONTAINER_ERROR_BUFFEROVERFLOW  -20
#define CONTAINER_ERROR_DIVISION_BY_ZERO -21

typedef void (*ErrorFunction)(const char *,int,...);
typedef int (*DestructorFunction)(void *);

typedef struct tagError {
    ErrorFunction RaiseError;
    void (*EmptyErrorFunction)(const char *fname,int errcode,...);
    char *(*StrError)(int errcode);
    ErrorFunction (*SetErrorFunction)(ErrorFunction);
    int (*NullPtrError)(char *fname);
} ErrorInterface;

extern ErrorInterface iError;
/************************************************************************** */
/*                                                                          */
/*                          Memory allocation object                        */
/*                                                                          */
/************************************************************************** */
typedef struct tagMemoryManager {
    void *(*malloc)(size_t);        /* Function to allocate memory */
    void (*free)(void *);           /* Function to release it      */
    void *(*realloc)(void *,size_t);/* Function to resize a block of memory */
    void *(*calloc)(size_t,size_t);
} ContainerMemoryManager;
extern ContainerMemoryManager * CurrentMemoryManager;
ContainerMemoryManager *SetCurrentMemoryManager(ContainerMemoryManager *in);
extern ContainerMemoryManager iDebugMalloc;

/****************************************************************************
 *                   Masks interface                                        *
 ****************************************************************************/
typedef struct _Mask Mask;
typedef struct tagMaskInterface {
    int (*And)(Mask *src1,Mask *src2);
    int (*Or)(Mask *src1,Mask *src2);
    int (*Not)(Mask *src);
    Mask *(*CreateFromMask)(size_t n,char *data);
    Mask *(*Create)(size_t n);
    Mask *(*Copy)(Mask *src);
    size_t (*Size)(Mask *);
    size_t (*Sizeof)(Mask *);
    int (*Set)(Mask *m,size_t idx,int val);
    int (*Clear)(Mask *m);
    int (*Finalize)(Mask *m);
} MaskInterface;

extern MaskInterface iMask;
/************************************************************************** */
/*                         Iterator objects                                 */
/************************************************************************** */

typedef struct _Iterator {
    void *(*GetNext)(struct _Iterator *);
    void *(*GetPrevious)(struct _Iterator *);
    void *(*GetFirst)(struct _Iterator *);
    void *(*GetCurrent)(struct _Iterator *);
    void *(*GetLast)(struct _Iterator *);
    void *(*Seek)(struct _Iterator *,size_t);
} Iterator;

/* Type definition of the compare function */
typedef struct tagCompareInfo {
    void *ExtraArgs;
    void *ContainerLeft;
    void *ContainerRight;
} CompareInfo;
typedef int (*CompareFunction)(const void *elem1, const void *elem2,CompareInfo *ExtraArgs);
typedef size_t (*SaveFunction)(const void *element, void *arg, FILE *OutputStream);
typedef size_t (*ReadFunction)(void *element, void *arg, FILE *InputStream);

typedef struct tagErrorInfo {
    int ErrorCode;
    const char *OperationName;
} ErrorInfo;

/************************************************************************** */
/*                                                                          */
/*                      Heap allocator objects                              */
/*                                                                          */
/************************************************************************** */
typedef struct tagHeapObject ContainerHeap;
typedef struct _HeapAllocatorInterface {
    ContainerHeap *(*Create)(size_t ElementSize,ContainerMemoryManager *m);
    void *(*newObject)(ContainerHeap *heap);
    void (*AddToFreeList)(ContainerHeap *heap,void *element);
    void (*Clear)(ContainerHeap *heap);
    void (*Finalize)(ContainerHeap *heap);
    ContainerHeap *(*InitHeap)(void *heap,size_t nbElements,ContainerMemoryManager *allocator);
    size_t (*Sizeof)(ContainerHeap *heap);
    Iterator *(*NewIterator)(ContainerHeap *);
    int (*deleteIterator)(Iterator *it);
} HeapInterface;

extern HeapInterface iHeap;

/************************************************************************** */
/*                                                                          */
/*                            Pool Allocator object interface               */
/* This interface comes in two flavors: normal and debug versions           */
/************************************************************************** */
typedef struct Pool Pool;
typedef struct _tagPoolAllocatorInterface {
    Pool  *(*Create)(ContainerMemoryManager *m);
    void  *(*Alloc)(Pool *pool,size_t size);
    void  *(*Calloc)(Pool *pool,size_t n,size_t size);
    void   (*Clear)(Pool *);
    void   (*Destroy)(Pool *);
} PoolAllocatorInterface;

extern PoolAllocatorInterface iPool;

typedef struct _tagPoolAllocatorDebugInterface {
    Pool  *(*Create)(const char *file_line);
    void  *(*Alloc)(Pool *pool,size_t size,const char *file_line);
    void  *(*Calloc)(Pool *pool,size_t n,size_t size,const char *file_line);
    void   (*Clear)(Pool *,const char *file_line);
    void   (*Finalize)(Pool *,const char *file_line);
    int    (*FindPoolFromData)(Pool *pool, void *data);
    void   (*SetMaxFree)(Pool *pool, size_t size);
    size_t (*Sizeof)(Pool *);
} PoolAllocatorDebugInterface;

extern PoolAllocatorDebugInterface iPoolDebug;
/************************************************************************** */
/*                                                                          */
/*                            Generic containers                            */
/*                                                                          */
/************************************************************************** */
typedef struct _GenericContainer GenericContainer;
typedef struct tagGenericContainerInterface {
    size_t (*Size)(GenericContainer *Gen);
    unsigned (*GetFlags)(GenericContainer *Gen);
    unsigned (*SetFlags)(GenericContainer *Gen,unsigned flags);
    int (*Clear)(GenericContainer *Gen);
    int (*Contains)(GenericContainer *Gen,void *Value);
    int (*Erase)(GenericContainer *Gen,void *);
    int (*Finalize)(GenericContainer *Gen);
    void (*Apply)(GenericContainer *Gen,int (*Applyfn)(void *,void * arg),void *arg);
    int (*Equal)(GenericContainer *Gen1,GenericContainer *Gen2);
    GenericContainer *(*Copy)(GenericContainer *Gen);
    ErrorFunction (*SetErrorFunction)(GenericContainer *Gen,ErrorFunction fn);
    size_t (*Sizeof)(GenericContainer *Gen);
    Iterator *(*NewIterator)(GenericContainer *Gen);
    int (*deleteIterator)(Iterator *);
    int (*Save)(GenericContainer *Gen,FILE *stream, SaveFunction saveFn,void *arg);
} GenericContainerInterface;
extern GenericContainerInterface iGeneric;

/************************************************************************** */
/*                                                                          */
/*                            Sequential Containers                         */
/*                                                                          */
/************************************************************************** */
typedef struct SequentialContainer SequentialContainer;
typedef struct tagSequentialContainerInterface {

    size_t (*Size)(SequentialContainer *Gen);
    unsigned (*GetFlags)(SequentialContainer *Gen);
    unsigned (*SetFlags)(SequentialContainer *Gen,unsigned flags);
    int (*Clear)(SequentialContainer *Gen);
    int (*Contains)(SequentialContainer *Gen,void *Value);
    int (*Erase)(SequentialContainer *Gen,void *);
    int (*Finalize)(SequentialContainer *Gen);
    void (*Apply)(SequentialContainer *Gen,int (*Applyfn)(void *,void * arg),void *arg);
    int (*Equal)(SequentialContainer *Gen1,SequentialContainer *Gen2);
    SequentialContainer *(*Copy)(SequentialContainer *Gen);
    ErrorFunction (*SetErrorFunction)(SequentialContainer *Gen,ErrorFunction fn);
    size_t (*Sizeof)(SequentialContainer *Gen);
    Iterator *(*NewIterator)(SequentialContainer *Gen);
    int (*deleteIterator)(Iterator *);
    int (*Save)(SequentialContainer *Gen,FILE *stream, SaveFunction saveFn,void *arg);


    int (*Add)(SequentialContainer *SC,void *Element);
    void *(*GetElement)(SequentialContainer *SC,size_t idx);
    int (*Push)(SequentialContainer *Gen,void *Element);
    int (*Pop)(SequentialContainer *Gen,void *result);
    int (*InsertAt)(SequentialContainer *SC,size_t idx, void *newval);
    int (*EraseAt)(SequentialContainer *SC,size_t idx);
    int (*ReplaceAt)(SequentialContainer *SC, size_t idx, void *element);
    int (*IndexOf)(SequentialContainer *SC,void *ElementToFind,void *args,size_t *result);
    int (*Append)(SequentialContainer *SC1,SequentialContainer *SC2);
} SequentialContainerInterface;
extern SequentialContainerInterface iSequentialContainer;

/************************************************************************** */
/*                                                                          */
/*                           Associative Containers                         */
/*                                                                          */
/************************************************************************** */
typedef struct AssociativeContainer AssociativeContainer;
typedef struct tagAssociativeContainerInterface {
    GenericContainerInterface Generic;
    int (*Add)(SequentialContainer *SC,void *key,void *Element);
    void *(*GetElement)(AssociativeContainer *SC,void *Key);
    int (*Replace)(AssociativeContainer *SC, void *Key, void *element);
} AssociativeContainerInterface;
extern AssociativeContainerInterface iAssociativeContainer;
/************************************************************************** */
/*                                                                          */
/*                            Stream Buffers                                */
/*                                                                          */
/************************************************************************** */
typedef struct _StreamBuffer StreamBuffer;
typedef struct tagStreamBufferInterface {
    StreamBuffer *(*Create)(size_t startsize);
    StreamBuffer *(*CreateWithAllocator)(size_t startsize, 
                                         ContainerMemoryManager *allocator);
    size_t (*Read)(StreamBuffer *b, void *data, size_t siz);
    size_t (*Write)(StreamBuffer *b,void *data, size_t siz);
    int (*SetPosition)(StreamBuffer *b,size_t pos);
    size_t (*GetPosition)(StreamBuffer *b);
    char *(*GetData)(StreamBuffer *b);
    size_t (*Size)(StreamBuffer *b);
    int (*Clear)(StreamBuffer *b);
    int (*Finalize)(StreamBuffer *b);
    int (*Resize)(StreamBuffer *b,size_t newSize);
} StreamBufferInterface;
extern StreamBufferInterface iStreamBuffer;
/************************************************************************** */
/*                                                                          */
/*                            circular buffers                              */
/*                                                                          */
/************************************************************************** */
typedef struct _CircularBuffer CircularBuffer;
typedef struct tagCircularBufferInterface {
    size_t (*Size)(CircularBuffer *cb);
    int (*Add)( CircularBuffer * b, void *data_element);
    int (*PopFront)(CircularBuffer *b,void *result);
    int (*PeekFront)(CircularBuffer *b,void *result);
    CircularBuffer *(*CreateWithAllocator)(size_t sizElement,
                        size_t sizeBuffer,ContainerMemoryManager *allocator);
    CircularBuffer *(*Create)(size_t sizElement,size_t sizeBuffer);
    int (*Clear)(CircularBuffer *cb);
    int (*Finalize)(CircularBuffer *cb);
    size_t (*Sizeof)(CircularBuffer *cb);
    DestructorFunction(*SetDestructor)(CircularBuffer *cb,
                                       DestructorFunction fn);
} CircularBufferInterface;
extern CircularBufferInterface iCircularBuffer;
/************************************************************************** */
/*                                                                          */
/*                            String Collections                            */
/*                                                                          */
/************************************************************************** */
typedef struct strCollection strCollection;
typedef struct _Vector Vector;

typedef int (*StringCompareFn)(const char *s1,const char *s2,CompareInfo *info);
typedef struct tagstrCollection {
/* -----------------------------------------------This is the generic container part */
    /* Returns the number of elements stored */
    size_t (*Size)(strCollection *SC);
    /* Flags for this container */
    unsigned (*GetFlags)(const strCollection *SC);
    /* Sets this collection read-only or unsets the read-only flag */
    unsigned (*SetFlags)(strCollection *SC,unsigned flags);
    /* Clears all data and frees the memory */
    int (*Clear)(strCollection *SC);
    /*Case sensitive search of a character string in the data */
    int (*Contains)(strCollection *SC,char *str);
    /* erases the given string if found */
    int (*Erase)(strCollection *SC,char *);
    /* Frees the memory used by the collection */
    int (*Finalize)(strCollection *SC);
    /* Calls the given function for all strings. "Arg" is a used supplied argument */
    /* that can be NULL that is passed to the function to call */
    int (*Apply)(strCollection *SC,int (*Applyfn)(char *,void * arg),void *arg);
    /* Compares two string collections */
    int (*Equal)(strCollection *SC1,strCollection *SC2);
    /* Copy a string collection */
    strCollection *(*Copy)(strCollection *SC);
    /* Set or unset the error function */
    ErrorFunction (*SetErrorFunction)(strCollection *SC,ErrorFunction fn);
    /* Memory used by the string collection object and its data */
    size_t (*Sizeof)(strCollection *SC);
    Iterator *(*NewIterator)(strCollection *SC);
    int (*deleteIterator)(Iterator *);
    /* Writes the string collection in binary form to a stream */
    int (*Save)(strCollection *SC,FILE *stream, SaveFunction saveFn,void *arg);
    strCollection *(*Load)(FILE *stream, ReadFunction readFn,void *arg);
    size_t (*GetElementSize)(const strCollection *SC);
 /* -------------------------------------------This is the Sequential container part */
    /* Adds one element at the end. Given string is copied */
    int (*Add)(strCollection *SC,char *newval);
    /* Pushes a string, using the collection as a stack */
    int (*PushFront)(strCollection *SC,char *str);
    /* Pops the first (position zero) string of the collection */
    size_t (*PopFront)(strCollection *SC,char *outbuf,size_t buflen);
    /* Inserts a string at the given position */
    int (*InsertAt)(strCollection *SC,size_t idx,char *newval);
    /*erases the string at the indicated position */
    int (*EraseAt)(strCollection *SC,size_t idx);
    /* Replaces the character string at the given position with a new one */
    int (*ReplaceAt)(strCollection *SC,size_t idx,char *newval);
    /*Returns the index of the given string or -1 if not found */
   int (*IndexOf)(strCollection *SC,char *SearchedString,size_t *result);   
 /* ---------------------------------------------This is the specific container part */
    bool (*Sort)(strCollection *SC);
    struct _Vector *(*CastToArray)(strCollection *SC);
    size_t (*FindFirst)(strCollection *SC,char *text);
    size_t (*FindNext)(strCollection *SC, char *text,size_t start);
    strCollection *(*FindText)(strCollection *SC,char *text);
    Vector *(*FindTextIndex)(strCollection *SC,char *text);
    Vector *(*FindTextPositions)(strCollection *SC,char *text);
    int (*WriteToFile)(strCollection *SC, char *filename);
    strCollection *(*IndexIn)(const strCollection *SC,const Vector *AL);
    strCollection *(*CreateFromFile)(const char *fileName);
    strCollection *(*Create)(size_t startsize);
    strCollection *(*CreateWithAllocator)(size_t startsize,ContainerMemoryManager *allocator);

    /* Adds a NULL terminated table of strings */
    int (*AddRange)(strCollection *SC,size_t n,char **newvalues);
    /* Copies all strings into a NULL terminated vector */
    char **(*CopyTo)(strCollection *SC);

    /* Inserts a string at the position zero. */
    int (*Insert)(strCollection *SC,char *);
    int (*InsertIn)(strCollection *source, size_t idx, strCollection *newData);
    /* Returns the string at the given position */
    char *(*GetElement)(const strCollection *SC,size_t idx);
    /* Returns the current capacity of the collection */
    size_t (*GetCapacity)(strCollection *SC);
    /* Sets the capacity if there are no items in the collection */
    int (*SetCapacity)(strCollection *SC,size_t newCapacity);
    StringCompareFn (*SetCompareFunction)(strCollection *SC,StringCompareFn);
    int (*Reverse)(strCollection *SC);
    int (*Append)(strCollection *,strCollection *);
    size_t (*PopBack)(strCollection *, char *result,size_t bufsize);
    int (*PushBack)(strCollection *,char *data);
    strCollection *(*GetRange)(strCollection *SC, size_t start,size_t end);
    ContainerMemoryManager *(*GetAllocator)(strCollection *AL);
    int (*Mismatch)(strCollection *a1,strCollection *a2,size_t *mismatch);
    strCollection *(*InitWithAllocator)(strCollection *result,size_t startsize,ContainerMemoryManager *allocator);
    strCollection *(*Init)(strCollection *result,size_t startsize);
    DestructorFunction (*SetDestructor)(strCollection *v,DestructorFunction fn);
    strCollection *(*InitializeWith)(size_t n, char **data);
//    unsigned char *(*Find)(strCollection *SC,unsigned char *str,CompareInfo *ci);
} strCollectionInterface;

extern strCollectionInterface istrCollection;

/************************************************************************** */
/*                                                                          */
/*                            Wide String Collections                            */
/*                                                                          */
/************************************************************************** */
typedef struct WstrCollection WstrCollection;

typedef int (*WStringCompareFn)(const wchar_t *s1,const wchar_t *s2,CompareInfo *info);
typedef struct tagWstrCollection {
	/* -----------------------------------------------This is the generic container part */
    /* Returns the number of elements stored */
    size_t (*Size)(WstrCollection *SC);
    /* Flags for this container */
    unsigned (*GetFlags)(const WstrCollection *SC);
    /* Sets this collection read-only or unsets the read-only flag */
    unsigned (*SetFlags)(WstrCollection *SC,unsigned flags);
    /* Clears all data and frees the memory */
    int (*Clear)(WstrCollection *SC);
    /*Case sensitive search of a character string in the data */
    int (*Contains)(WstrCollection *SC,wchar_t *str);
    /* erases the given string if found */
    int (*Erase)(WstrCollection *SC,wchar_t *);
    /* Frees the memory used by the collection */
    int (*Finalize)(WstrCollection *SC);
    /* Calls the given function for all strings. "Arg" is a used supplied argument */
    /* that can be NULL that is passed to the function to call */
    int (*Apply)(WstrCollection *SC,int (*Applyfn)(wchar_t *,void * arg),void *arg);
    /* Compares two string collections */
    int (*Equal)(WstrCollection *SC1,WstrCollection *SC2);
    /* Copy a string collection */
    WstrCollection *(*Copy)(WstrCollection *SC);
    /* Set or unset the error function */
    ErrorFunction (*SetErrorFunction)(WstrCollection *SC,ErrorFunction fn);
    /* Memory used by the string collection object and its data */
    size_t (*Sizeof)(WstrCollection *SC);
    Iterator *(*NewIterator)(WstrCollection *SC);
    int (*deleteIterator)(Iterator *);
    /* Writes the string collection in binary form to a stream */
    int (*Save)(WstrCollection *SC,FILE *stream, SaveFunction saveFn,void *arg);
    WstrCollection *(*Load)(FILE *stream, ReadFunction readFn,void *arg);
    size_t (*GetElementSize)(const WstrCollection *SC);
	/* -------------------------------------------This is the Sequential container part */
    /* Adds one element at the end. Given string is copied */
    int (*Add)(WstrCollection *SC,wchar_t *newval);
    /* Pushes a string, using the collection as a stack */
    int (*PushFront)(WstrCollection *SC,wchar_t *str);
    /* Pops the first (position zero) string of the collection */
    size_t (*PopFront)(WstrCollection *SC,wchar_t *outbuf,size_t buflen);
    /* Inserts a string at the given position */
    int (*InsertAt)(WstrCollection *SC,size_t idx,wchar_t *newval);
    /*erases the string at the indicated position */
    int (*EraseAt)(WstrCollection *SC,size_t idx);
    /* Replaces the character string at the given position with a new one */
    int (*ReplaceAt)(WstrCollection *SC,size_t idx,wchar_t *newval);
    /*Returns the index of the given string or -1 if not found */
    int (*IndexOf)(WstrCollection *SC,wchar_t *SearchedString,size_t *result);
	/* ---------------------------------------------This is the specific container part */
    bool (*Sort)(WstrCollection *SC);
    struct _Vector *(*CastToArray)(WstrCollection *SC);
    size_t (*FindFirst)(WstrCollection *SC,wchar_t *text);
    size_t (*FindNext)(WstrCollection *SC, wchar_t *text,size_t start);
    WstrCollection *(*FindText)(WstrCollection *SC,wchar_t *text);
    Vector *(*FindTextIndex)(WstrCollection *SC,wchar_t *text);
    Vector *(*FindTextPositions)(WstrCollection *SC,wchar_t *text);
    int (*WriteToFile)(WstrCollection *SC, char *filename);
    WstrCollection *(*IndexIn)(const WstrCollection *SC,const Vector *AL);
    WstrCollection *(*CreateFromFile)(const char *fileName);
    WstrCollection *(*Create)(size_t startsize);
    WstrCollection *(*CreateWithAllocator)(size_t startsize,ContainerMemoryManager *allocator);
	
    /* Adds a NULL terminated table of strings */
    int (*AddRange)(WstrCollection *SC,size_t n,wchar_t **newvalues);
    /* Copies all strings into a NULL terminated vector */
    wchar_t **(*CopyTo)(WstrCollection *SC);
	
    /* Inserts a string at the position zero. */
    int (*Insert)(WstrCollection *SC,wchar_t *);
    int (*InsertIn)(WstrCollection *source, size_t idx, WstrCollection *newData);
    /* Returns the string at the given position */
    wchar_t *(*GetElement)(const WstrCollection *SC,size_t idx);
    /* Returns the current capacity of the collection */
    size_t (*GetCapacity)(WstrCollection *SC);
    /* Sets the capacity if there are no items in the collection */
    int (*SetCapacity)(WstrCollection *SC,size_t newCapacity);
    WStringCompareFn (*SetCompareFunction)(WstrCollection *SC,WStringCompareFn);
    int (*Reverse)(WstrCollection *SC);
    int (*Append)(WstrCollection *,WstrCollection *);
    size_t (*PopBack)(WstrCollection *,wchar_t *result,size_t bufsize);
    int (*PushBack)(WstrCollection *,wchar_t *data);
    WstrCollection *(*GetRange)(WstrCollection *SC, size_t start,size_t end);
    ContainerMemoryManager *(*GetAllocator)(WstrCollection *AL);
    int (*Mismatch)(WstrCollection *a1,WstrCollection *a2,size_t *mismatch);
    WstrCollection *(*InitWithAllocator)(WstrCollection *result,size_t startsize,ContainerMemoryManager *allocator);
    WstrCollection *(*Init)(WstrCollection *result,size_t startsize);
    DestructorFunction (*SetDestructor)(WstrCollection *v,DestructorFunction fn);
    WstrCollection *(*InitializeWith)(size_t n,wchar_t **data);
//    wchar_t *Find(WstrCollection *SC,wchar_t *data,CompareInfo *ci);
} WstrCollectionInterface;

extern WstrCollectionInterface iWstrCollection;



/* -------------------------------------------------------------------------
 * LIST Interface                                                           *
 * This describes the single linked list data structure functions          *
 * Each list objects has a list composed of list elements.                 *
 *-------------------------------------------------------------------------*/
typedef struct _List List;
typedef struct tagList {
    size_t (*Size)(List *L);        /* Returns the number of elements stored */
    unsigned (*GetFlags)(List *L);                      /* Gets the flags */
    unsigned (*SetFlags)(List *L,unsigned flags);       /* Sets the flags */
    int (*Clear)(List *L);                         /* Clears all elements */
    int (*Contains)(List *L,void *element);       /* Searches an element */
    int (*Erase)(List *L,void *);       /* erases the given data if found */
    /* Frees the memory used by the collection and destroys the list */
    int (*Finalize)(List *L);
    /* Call a callback function with each element of the list and an extra argument */
    int (*Apply)(List *L,int(Applyfn)(void *,void *),void *arg);
    int (*Equal)(List *l1,List *l2);  /* Compares two lists (Shallow comparison) */
    List *(*Copy)(List *L);           /* Copies all items into a new list */
    ErrorFunction (*SetErrorFunction)(List *L,ErrorFunction); /* Set/unset the error function */
    size_t (*Sizeof)(List *l);
    Iterator *(*NewIterator)(List *L);
    int (*deleteIterator)(Iterator *);
    int (*Save)(List *L,FILE *stream, SaveFunction saveFn,void *arg);
    List *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    size_t (*GetElementSize)(List *l);
 /* -------------------------------------------This is the Sequential container part */
    int (*Add)(List *L,void *newval);              /* Adds element at end */
    /* Returns the data at the given position */
    void *(*GetElement)(List *L,size_t idx);
    /* Pushes an element, using the collection as a stack */
    int (*PushFront)(List *L,void *str);
    /* Pops the first element the list */
    int (*PopFront)(List *L,void *result);
    /* Inserts a value at the given position */
    int (*InsertAt)(List *L,size_t idx,void *newval);
    int (*EraseAt)(List *L,size_t idx);
    /* Replaces the element at the given position with a new one */
    int (*ReplaceAt)(List *L,size_t idx,void *newval);
    /*Returns the index of the given data or -1 if not found */
    int (*IndexOf)(List *L,void *SearchedElement,void *ExtraArgs,size_t *result);
/* -------------------------------------------This is the list container part */
    int (*InsertIn)(List *l, size_t idx,List *newData);
    int  (*CopyElement)(List *list,size_t idx,void *OutBuffer);
    /*erases the string at the indicated position */
    int (*EraseRange)(List *L,size_t start,size_t end);
    /* Sorts the list in place */
    int (*Sort)(List *l);
    /* Reverses the list */
    int (*Reverse)(List *l);
    /* Gets an element range from the list */
    List *(*GetRange)(List *l,size_t start,size_t end);
    /* Add a list at the end of another */
    int (*Append)(List *l1,List *l2);
    /* Set the comparison function */
    CompareFunction (*SetCompareFunction)(List *l,CompareFunction fn);
    CompareFunction Compare;
    int (*UseHeap)(List *L, ContainerMemoryManager *m);
    int (*AddRange)(List *L, size_t n,void *data);
    List *(*Create)(size_t element_size);
    List *(*CreateWithAllocator)(size_t elementsize,ContainerMemoryManager *allocator);
    List *(*Init)(List *aList,size_t element_size);
    List *(*InitWithAllocator)(List *aList,size_t element_size,ContainerMemoryManager *allocator);
    List *(*SetAllocator)(List *l, ContainerMemoryManager  *allocator);
    int (*InitIterator)(List *list,void *storage);
    ContainerMemoryManager *(*GetAllocator)(List *list);
    DestructorFunction (*SetDestructor)(List *v,DestructorFunction fn);
    List *(*InitializeWith)(size_t elementSize,size_t n,void *data);
} ListInterface;

extern ListInterface iList;
/* -------------------------------------------------------------------
 *                           QUEUES
 * -------------------------------------------------------------------*/
typedef struct _Queue Queue;

typedef struct _QueueInterface {
    Queue *(*Create)(size_t elementSize);
    Queue *(*CreateWithAllocator)(size_t elementSize,
                                  ContainerMemoryManager *allocator);
    size_t (*Size)(Queue *Q);
    size_t (*Sizeof)(Queue *Q);
    int (*Enqueue)(Queue *Q, void *Element);
    int (*Dequeue)(Queue *Q,void *result);
    int (*Clear)(Queue *Q);
    int (*Finalize)(Queue *Q);
    int (*Front)(Queue *Q,void *result);
    int (*Back)(Queue *Q,void *result);
    List *(*GetList)(Queue *q);
} QueueInterface;

extern QueueInterface iQueue;
/* --------------------------------------------------------------------
 *                           Double Ended QUEues                      *
 * -------------------------------------------------------------------*/
typedef struct deque_t Deque;
typedef struct _DeQueueInterface {
    size_t (*Size)(Deque *Q);
    unsigned (*GetFlags)(Deque *Q);
    unsigned (*SetFlags)(Deque *Q,unsigned newFlags);
    int (*Clear)(Deque *Q);
    size_t (*Contains)(Deque * d, void* item);
    int (*Erase)(Deque * d, void* item);
    int (*Finalize)(Deque *Q);
    void (*Apply)(Deque *Q,int (*Applyfn)(void *,void * arg),void *arg);
    int (*Equal)(Deque *d1,Deque *d2);
    Deque *(*Copy)(Deque *d);
    ErrorFunction (*SetErrorFunction)(Deque *d,ErrorFunction); 
    size_t (*Sizeof)(Deque *d);
    Iterator *(*NewIterator)(Deque *Deq);
    int (*deleteIterator)(Iterator *);
    int (*Save)(Deque *d,FILE *stream, SaveFunction saveFn,void *arg);
    Deque *(*Load)(FILE *stream, ReadFunction readFn,void *arg);
    /* Deque specific functions */
    int (*PushBack)(Deque *Q, void *Element);
    int (*PushFront)(Deque *Q, void *Element);
    int (*Reverse)(Deque * d);
    int (*PopBack)(Deque *d,void *outbuf);
    int (*Back)(Deque *d,void *outbuf);
    int (*Front)(Deque *d,void *outbuf);
    int (*PopFront)(Deque *d,void *outbuf);
    Deque *(*Create)(size_t elementSize);
    Deque *(*Init)(Deque *d,size_t elementSize);
    DestructorFunction (*SetDestructor)(Deque *Q,DestructorFunction fn);
} DequeInterface;

extern DequeInterface iDeque;

/* ------------------------------------------------------------------------
 * Double linked list interface                                            *
 * This describes the double linked list data structure functions          *
 * Each list objects has a list composed of list elemants.                 *
 *-------------------------------------------------------------------------*/
typedef struct Dlist Dlist;
typedef struct tagDlist {
    size_t (*Size)(Dlist *dl);             /* Returns the number of elements stored */
    unsigned (*GetFlags)(Dlist *AL);                               /* Gets the flags */
    unsigned (*SetFlags)(Dlist *AL,unsigned flags);                    /* Sets flags */
    int (*Clear)(Dlist *dl);                 /* Clears all data and frees the memory */
    int (*Contains)(Dlist *dl,void *element);                /* Searches an element */
    int (*Erase)(Dlist *AL,void *);                /* erases the given data if found */
    int (*Finalize)(Dlist *AL);                                 /* Frees memory used */
                 /* Call a callback function with each element and an extra argument */
    int (*Apply)(Dlist *L,int(Applyfn)(void *elem,void *extraArg),void *extraArg);
    bool (*Equal)(Dlist *l1,Dlist *l2);   /* Compares two lists (Shallow comparison) */
    Dlist *(*Copy)(Dlist *dl);                  /* Copies all items into a new Dlist */
    ErrorFunction (*SetErrorFunction)(Dlist *L,ErrorFunction);
    size_t (*Sizeof)(Dlist *dl);
    Iterator *(*NewIterator)(Dlist *);
    int (*deleteIterator)(Iterator *);
    int (*Save)(Dlist *L,FILE *stream, SaveFunction saveFn,void *arg);
    Dlist *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    size_t (*GetElementSize)(Dlist *dl);

    /* Sequential container part */
    int (*Add)(Dlist *dl,void *newval);              /* Adds one element at the end. */
    void *(*GetElement)(Dlist *AL,int idx);             /* Returns  data at position */
    int (*PushFront)(Dlist *AL,void *str);             /* Pushes an element at start */
    int (*PopFront)(Dlist *AL,void *result);     /* Pops the first element the Dlist */
    int (*InsertAt)(Dlist *AL,size_t idx,void *newval); /* Inserts value at position */
    int (*EraseAt)(Dlist *AL,size_t idx);              /* erases element at position */
    int (*ReplaceAt)(Dlist *AL,size_t idx,void *newval); /* Replace element at pos */
    int (*IndexOf)(Dlist *AL,void *SearchedElement,void *args,size_t *result); 

    /* Dlist container specific part */
    int (*PushBack)(Dlist *AL,void *str);                /* Pushes an element at end */
    int (*PopBack)(Dlist *AL,void *result);       /* Pops the last element the Dlist */
                                                      /* Inserts a list into another */
    Dlist *(*Splice)(Dlist *list,void *pos,Dlist *toInsert,int direction);
    int (*Sort)(Dlist *l);                                        /* Sorts the list */
    int (*Reverse)(Dlist *l);                                /* Reverses the list */
    Dlist *(*GetRange)(Dlist *l,size_t start,size_t end);            /* Gets a range */
    int (*Append)(Dlist *l1,Dlist *l2);         /* Add a Dlist at the end of another */
                                       /* Sets the comparison function for this list */
    CompareFunction (*SetCompareFunction)(Dlist *l,CompareFunction fn);
    int (*UseHeap)(Dlist *L, ContainerMemoryManager *m);
    int (*AddRange)(Dlist *l,size_t n,void *data);
    Dlist *(*Create)(size_t elementsize);
    Dlist *(*CreateWithAllocator)(size_t,ContainerMemoryManager *);
    Dlist *(*Init)(Dlist *dlist,size_t elementsize);
    int (*CopyElement)(Dlist *l,size_t idx,void *outbuf);/* Copies element to buffer */
    int (*InsertIn)(Dlist *l, size_t idx,Dlist *newData);/* Inserts list at position */
    DestructorFunction (*SetDestructor)(Dlist *v,DestructorFunction fn);
    Dlist *(*InitializeWith)(size_t elementSize, size_t n,void *data);

} DlistInterface;

extern DlistInterface iDlist;

/****************************************************************************
 *           Vectors                                                        *
 ****************************************************************************/

/* Definition of the functions associated with this type. */
typedef struct tagVector {
    /* Returns the number of elements stored */
    size_t (*Size)(const Vector *AL);
    /* Is this collection read only? */
    unsigned (*GetFlags)(const Vector *AL);
    /* Sets this collection read-only or unsets the read-only flag */
    unsigned (*SetFlags)(Vector *AL,unsigned flags);
    /* Clears all data and frees the memory */
    int (*Clear)(Vector *AL);
    /*Case sensitive search of a character string in the data */
    int (*Contains)(Vector *AL,void *str,void *ExtraArgs);
    /* erases the given string if found */
    int (*Erase)(Vector *AL,void *);
    /* Frees the memory used by the collection */
    int (*Finalize)(Vector *AL);
    /* Calls the given function for all strings in the given collection.
       "Arg" is a user supplied argument (can be NULL) that is passed
       to the function to call */
    int (*Apply)(Vector *AL,int (*Applyfn)(void *element,void * arg),void *arg);
    int (*Equal)(Vector *first,Vector *second);
        /* Copies an array list */
    Vector *(*Copy)(Vector *AL);
    ErrorFunction (*SetErrorFunction)(Vector *AL,ErrorFunction);
    size_t (*Sizeof)(Vector *AL);
    Iterator *(*NewIterator)(Vector *AL);
    int (*deleteIterator)(Iterator *);
    /* Writes the vector in binary form to a stream */
    int (*Save)(Vector *AL,FILE *stream, SaveFunction saveFn,void *arg);
    Vector *(*Load)(FILE *stream, ReadFunction readFn,void *arg);
    size_t (*GetElementSize)(const Vector *AL);

    /* Sequential container specific fields */

    /* Adds one element at the end. Given element is copied */
    int (*Add)(Vector *AL,void *newval);
    /* Returns the string at the given position */
    void *(*GetElement)(const Vector *AL,size_t idx);
    /* Pushes a string, using the collection as a stack */
    int (*PushBack)(Vector *AL,void *str);
    /* Pops the last string off the collection */
    int (*PopBack)(Vector *AL,void *result);
    /* Inserts an element at the given position */
    int (*InsertAt)(Vector *AL,size_t idx,void *newval);
    /*erases the string at the indicated position */
    int (*EraseAt)(Vector *AL,size_t idx);
    /* Replaces the character string at the given position with a new one */
    int (*ReplaceAt)(Vector *AL,size_t idx,void *newval);
    /*Returns the 1 based index of the given string or 0 if not found */
    int (*IndexOf)(Vector *AL,void *data,void *ExtraArgs,size_t *result);

    /* Vector container specific fields */

    int (*Insert)(Vector *AL,void *);
    int (*InsertIn)(Vector *AL, size_t idx, Vector *newData);
    Vector *(*IndexIn)(Vector *SC,Vector *AL);
    /* Returns the current capacity of the collection */
    size_t (*GetCapacity)(const Vector *AL);
    /* Sets the capacity in the collection */
    int (*SetCapacity)(Vector *AL,size_t newCapacity);

    CompareFunction (*SetCompareFunction)(Vector *l,CompareFunction fn);
    int (*Sort)(Vector *AL);
    Vector *(*Create)(size_t elementsize,size_t startsize);
    Vector *(*CreateWithAllocator)(size_t elementsize,size_t startsize,ContainerMemoryManager *allocator);
    Vector *(*Init)(Vector *r,size_t elementsize,size_t startsize);
    int (*AddRange)(Vector *AL,size_t n, void *newvalues);
    Vector *(*GetRange)(const Vector *AL, size_t start, size_t end);
    int (*CopyElement)(Vector *AL,size_t idx,void *outbuf);
    void **(*CopyTo)(Vector *AL);
    int (*Reverse)(Vector *AL);
    int (*Append)(Vector *AL1, Vector *AL2);
    int (*Mismatch)(Vector *a1,Vector *a2,size_t *mismatch);
    ContainerMemoryManager *(*GetAllocator)(Vector *AL);
    DestructorFunction (*SetDestructor)(Vector *v,DestructorFunction fn);
    int (*SearchWithKey)(Vector *vec,size_t startByte,size_t sizeKey,size_t startIndex,void *item,size_t *result);
    int (*Select)(Vector *src,Mask *m);
    Vector *(*SelectCopy)(Vector *src,Mask *m);
    int (*Resize)(Vector *AL,size_t newcapacity);
    Vector *(*InitializeWith)(size_t elementSize, size_t n, void *Data);
} VectorInterface;

extern VectorInterface iVector;

#include "valarray.h"


/* ----------------------------------Dictionary interface -----------------------*/

typedef struct _Dictionary Dictionary;

typedef struct tagDictionary {
    /* Returns the number of elements stored */
    size_t (*Size)(Dictionary *Dict);
    /* Get the flags */
    unsigned (*GetFlags)(Dictionary *Dict);
    /* Sets the flags */
    unsigned (*SetFlags)(Dictionary *Dict,unsigned flags);
    /* Clears all data and frees the memory */
    int (*Clear)(Dictionary *Dict);
    int (*Contains)(Dictionary *dict,const char *key);
    /* erases the given element if found */
    int (*Erase)(Dictionary *Dict,const char *);
    /* Frees the memory used by the dictionary */
    int (*Finalize)(Dictionary *Dict);
    /* Calls the given function for all objects. "Arg" is a used supplied argument */
    /* (that can be NULL) that is passed to the function to call */
    int (*Apply)(Dictionary *Dict,int (*Applyfn)(const char *Key,
                                                  const void *data,void *arg),void *arg);
    int (*Equal)(const Dictionary *d1,const Dictionary *d2);
    Dictionary *(*Copy)(Dictionary *dict);
    /* Set or unset the error function */
    ErrorFunction (*SetErrorFunction)(Dictionary *Dict,ErrorFunction fn);
    size_t (*Sizeof)(Dictionary *dict);
    Iterator *(*NewIterator)(Dictionary *dict);
    int (*deleteIterator)(Iterator *);
    int (*Save)(Dictionary *Dict,FILE *stream, SaveFunction saveFn,void *arg);
    Dictionary * (*Load)(FILE *stream, ReadFunction readFn, void *arg);
    size_t (*GetElementSize)(Dictionary *d);

    /* Hierarchical container specific fields */

    /* Adds one element. Given string is copied */
    int (*Add)(Dictionary *Dict,const char *key,void *Data);
    /* Returns the element at the given position */
    const void *(*GetElement)(const Dictionary *Dict,const char *Key);
    int (*Replace)(Dictionary *dict,const char *Key,void *Data);
    int (*Insert)(Dictionary *Dict,const char *key,void *Data);
    /* Dictionary specific fields */

    Vector *(*CastToArray)(Dictionary *);
    int (*CopyElement)(const Dictionary *Dict,const char *Key,void *outbuf);
    int (*InsertIn)(Dictionary *dst,Dictionary *src);
    Dictionary *(*Create)(size_t ElementSize,size_t hint);
    Dictionary *(*CreateWithAllocator)(size_t elementsize,size_t hint,ContainerMemoryManager *allocator);
    Dictionary *(*Init)(Dictionary *dict,size_t ElementSize,size_t hint);
    Dictionary *(*InitWithAllocator)(Dictionary *Dict,size_t elementsize,size_t hint,ContainerMemoryManager *allocator);
    strCollection *(*GetKeys)(Dictionary *Dict);
    ContainerMemoryManager *(*GetAllocator)(Dictionary *Dict);
    DestructorFunction (*SetDestructor)(Dictionary *v,DestructorFunction fn);	
    Dictionary *(*InitializeWith)(size_t elementSize,size_t n, char **Keys,void *Values);
} DictionaryInterface;

extern DictionaryInterface iDictionary;
/* ----------------------------------Hash table interface -----------------------*/
typedef struct _HashTable HashTable;
typedef unsigned int (*HashFunction)(const char *char_key, size_t *klen);

typedef struct tagHashTable {
    /* Construction */
    HashTable *(*Create)(size_t ElementSize);
    HashTable *(*Init)(HashTable *ht,size_t ElementSize);
    /* Returns the number of elements stored */
    size_t (*Size)(const HashTable *HT);
    size_t (*Sizeof)(const HashTable *HT);
    /* Get the flags */
    unsigned (*GetFlags)(const HashTable *HT);
    /* Sets th flags */
    unsigned (*SetFlags)(HashTable *HT,unsigned flags);
    /* Adds one element. Given string is copied */
    int (*Add)(HashTable *HT,const void *key,size_t klen,const void *Data);
    /* Clears all data and frees the memory */
    int (*Clear)(HashTable *HT);
    /* Returns the element with the given key */
    void *(*GetElement)(const HashTable *HT,const void *Key,size_t klen);
    int (*Search)(HashTable *ht,int (*Comparefn)(void *rec, const void *key,size_t klen,const void *value), void *rec);
    /* erases the given string if found */
    int (*Erase)(HashTable *HT,void *key,size_t klen);
    /* Frees the memory used by the HashTable */
    int (*Finalize)(HashTable *HT);
    /* Calls the given function for all strings. "Arg" is a used supplied argument */
    /* that can be NULL that is passed to the function to call */
    int (*Apply)(HashTable *HT,int (*Applyfn)(void *Key,size_t klen,void *data,void *arg),void *arg);
    /* Set or unset the error function */
    ErrorFunction (*SetErrorFunction)(HashTable *HT,ErrorFunction fn);
    int (*Resize)(HashTable *HT,size_t newSize);
    int (*Replace)(HashTable *HT,const void *key, size_t klen,const void *val);
    HashTable *(*Copy)(const HashTable *Orig,Pool *pool);
    HashFunction (*SetHashFunction)(HashTable *ht, HashFunction hf);
    /* Copy hash table base into overlay. If duplicates, table "base" wins*/
    HashTable *(*Overlay)(Pool *p, const HashTable *overlay, const HashTable *base);
    /*
     Merge two hash tables. If duplicate keys are found, call the "merger" callback to decide
     which element should be kept in the final hash table
     */
    HashTable *(*Merge)(Pool *p, const HashTable *overlay, const HashTable *base,
                        void * (*merger)(Pool *p,
                                         const void *key,
                                         size_t klen,
                                         const void *h1_val,
                                         const void *h2_val,
                                         const void *data),
                        const void *data);
    Iterator *(*NewIterator)(HashTable *);
    int (*deleteIterator)(Iterator *);
    int (*Save)(HashTable *HT,FILE *stream, SaveFunction saveFn,void *arg);
    HashTable *(*Load)(FILE *stream, ReadFunction readFn, void *arg);
    DestructorFunction (*SetDestructor)(HashTable *v,DestructorFunction fn);
} HashTableInterface;

extern HashTableInterface iHashTable;
void qsortEx(void *base, size_t num, size_t width,CompareFunction cmp, CompareInfo *ExtraArgs);

/* ---------------------------------------------------------------------------
 *                                                                           *
 *                         Binary searchtrees                                *
 * ------------------------------------------------------------------------  */
typedef struct tagBinarySearchTree BinarySearchTree;

typedef struct tagBinarySearchTreeInterface {
    /* Returns the number of elements stored */
    size_t (*Size)(BinarySearchTree *ST);
    /* Get the flags */
    unsigned (*GetFlags)(BinarySearchTree *ST);
    /* Sets the flags */
    unsigned (*SetFlags)(BinarySearchTree *ST, unsigned flags);
    /* Clears all data and frees the memory */
    int (*Clear)(BinarySearchTree *ST);
    /* Find a given data in the tree */
    int (*Contains)(BinarySearchTree *ST,void *data);
    /* erases the given data if found */
    int (*Erase)(BinarySearchTree *ST, const void *,void *);
    /* Frees the memory used by the tree */
    int (*Finalize)(BinarySearchTree *ST);
    /* Calls the given function for all nodes. "Arg" is a used supplied argument */
    /* that can be NULL that is passed to the function to call */
    int (*Apply)(BinarySearchTree *ST,int (*Applyfn)(const void *data,void *arg),void *arg);
    bool (*Equal)(const BinarySearchTree *left, const BinarySearchTree *right);
    /* Adds one element. Given data is copied */
    int (*Add)(BinarySearchTree *ST, const void *Data);
    /* Like Add but allows for an extra argument to be passed to the
       comparison function */
    int (*Insert)(BinarySearchTree * ST, const void *Data, void *ExtraArgs);
    /* Set or unset the error function */
    ErrorFunction (*SetErrorFunction)(BinarySearchTree *ST, ErrorFunction fn);
    CompareFunction (*SetCompareFunction)(BinarySearchTree *ST,CompareFunction fn);
    size_t (*Sizeof)(BinarySearchTree *ST);
    CompareFunction Compare;
    BinarySearchTree *(*Merge)(BinarySearchTree *left, BinarySearchTree *right, const void *data);
    Iterator *(*NewIterator)(BinarySearchTree *);
    int (*deleteIterator)(Iterator *);
    BinarySearchTree *(*Create)(size_t ElementSize);
    DestructorFunction (*SetDestructor)(BinarySearchTree *v,DestructorFunction fn);
} BinarySearchTreeInterface;

extern BinarySearchTreeInterface iBinarySearchTree;
/* -------------------------------------------------------------------------------------
 *                                                                                     *
 *                                Red Black trees                                      *
 * ----------------------------------------------------------------------------------  */

typedef struct tagRedBlackTree RedBlackTree;

typedef struct tagRedBlackTreeInterface {
    RedBlackTree *(*Create)(size_t ElementSize,size_t KeySize);
    size_t (*GetElementSize)(RedBlackTree *b);
    size_t (*Size)(RedBlackTree *ST); /* Returns the number of elements stored */
    unsigned (*GetFlags)(RedBlackTree *ST); /* Gets the flags */
    unsigned (*SetFlags)(RedBlackTree *ST, unsigned flags);
    /* Adds one element. Given data is copied */
    int (*Add)(RedBlackTree *ST, const void *Key,const void *Data);
    /* Like Add but allows for an extra argument to be passed to the
     comparison function */
    int (*Insert)(RedBlackTree *RB, const void *Key, const void *Data, void *ExtraArgs);
    /* Clears all data and frees the memory */
    int (*Clear)(RedBlackTree *ST);
    /* erases the given data if found */
    int (*Erase)(RedBlackTree *ST, const void *,void *);
    /* Frees the memory used by the tree */
    int (*Finalize)(RedBlackTree *ST);
    /* Calls the given function for all nodes. "Arg" is a used supplied argument */
    /* that can be NULL that is passed to the function to call */
    int (*Apply)(RedBlackTree *ST,int (*Applyfn)(const void *data,void *arg),void *arg);
    /* Find a given data in the tree */
    void *(*Find)(RedBlackTree *ST,void *key_data,void *ExtraArgs);
    /* Set or unset the error function */
    ErrorFunction (*SetErrorFunction)(RedBlackTree *ST, ErrorFunction fn);
    CompareFunction (*SetCompareFunction)(RedBlackTree *ST,CompareFunction fn);
    size_t (*Sizeof)(RedBlackTree *ST);
    CompareFunction Compare;
    Iterator *(*NewIterator)(RedBlackTree *);
    int (*deleteIterator)(Iterator *);
    DestructorFunction (*SetDestructor)(RedBlackTree *v,DestructorFunction fn);
} RedBlackTreeInterface;

extern RedBlackTreeInterface iRedBlackTree;
/* -------------------------------------------------------------------------------------
 *                                                                                     *
 *                            Balanced Binary trees                                    *
 *                                                                                     *
 * ----------------------------------------------------------------------------------  */
typedef struct tagTreeMap TreeMap;

typedef struct tagTreeMapInterface {
    size_t (*Size)(TreeMap *ST);   /* Returns the number of elements stored */
    unsigned (*GetFlags)(TreeMap *ST); /* Get the flags */
    unsigned (*SetFlags)(TreeMap *ST, unsigned flags); /* Sets the flags */
    int (*Clear)(TreeMap *ST); /* Clears all elements */
    int (*Contains)(TreeMap *ST,void *element,void *ExtraArgs);
    int (*Erase)(TreeMap *tree, void *element,void *ExtraArgs);  /* erases the given data if found */
    int (*Finalize)(TreeMap *ST);  /* Frees the memory used by the tree */
    int (*Apply)(TreeMap *ST,int (*Applyfn)(const void *data,void *arg),void *arg);
    int (*Equal)(TreeMap *t1, TreeMap *t2);
    TreeMap *(*Copy)(TreeMap *src);
    ErrorFunction (*SetErrorFunction)(TreeMap *ST, ErrorFunction fn);
    size_t (*Sizeof)(TreeMap *ST);
    Iterator *(*NewIterator)(TreeMap *);
    int (*deleteIterator)(Iterator *);
    int (*Save)(TreeMap *src,FILE *stream, SaveFunction saveFn,void *arg);
    int (*Add)(TreeMap *ST, void *Data,void *ExtraArgs); /* Adds one element. Given data is copied */
    int (*AddRange)(TreeMap *ST,size_t n, void *Data,void *ExtraArgs);
    /* Like Add but allows for an extra argument to be passed to the
     comparison function */
    int (*Insert)(TreeMap *RB, const void *Data, void *ExtraArgs);
    /* Calls the given function for all nodes. "Arg" is a used supplied argument */
    /* that can be NULL that is passed to the function to call */

    void *(*Find)(TreeMap *tree,void *element,void *ExtraArgs);
    CompareFunction (*SetCompareFunction)(TreeMap *ST,CompareFunction fn);
    TreeMap *(*CreateWithAllocator)(size_t ElementSize,ContainerMemoryManager *m);
    TreeMap *(*Create)(size_t ElementSize);
    size_t (*GetElementSize)(TreeMap *d);
    TreeMap *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    DestructorFunction (*SetDestructor)(TreeMap *v,DestructorFunction fn);
    TreeMap *(*InitializeWith)(size_t ElementSize, size_t n, void *data);
} TreeMapInterface;

extern TreeMapInterface iTreeMap;
/* -------------------------------------------------------------------------------------
 *                                                                                     *
 *                            Bit strings                                              *
 *                                                                                     *
 * ----------------------------------------------------------------------------------  */
typedef struct _BitString BitString;
#define BIT_TYPE unsigned char
#define BITSTRING_READONLY	1

/* Definition of the functions associated with this type. */
typedef struct tagBitString {
    size_t (*Size)(BitString *BitStr); /* Returns the number of elements stored */
    unsigned (*GetFlags)(BitString *BitStr); /* gets the flags */
    unsigned (*SetFlags)(BitString *BitStr,unsigned flags);/* Sets flags */
    /* Clears all data and frees the memory */
    int (*Clear)(BitString *BitStr);
    /* Searches a bit in the data */
    int (*Contains)(BitString *BitStr,BitString *str,void *ExtraArgs);
    /* erases the given string if found */
    int (*Erase)(BitString *BitStr,bool bit);
    /* Frees the memory used by the collection */
    int         (*Finalize)(BitString *BitStr);
    /* Calls the given function for all strings in the given collection.
     "Arg" is a user supplied argument (can be NULL) that is passed
     to the function to call */
    int        (*Apply)(BitString *BitStr,int (*Applyfn)(bool ,void * arg),void *arg);
    int       (*Equal)(BitString *bsl,BitString *bsr);
    BitString *(*Copy)(BitString *);
    ErrorFunction *(*SetErrorFunction)(BitString *,ErrorFunction fn);
    size_t     (*Sizeof)(BitString *b);
    Iterator  *(*NewIterator)(BitString *);
    int (*deleteIterator)(Iterator *);
    int        (*Save)(BitString *bitstr,FILE *stream, SaveFunction saveFn,void *arg);
    BitString *(*Load)(FILE *stream, ReadFunction saveFn,void *arg);
    size_t (*GetElementSize)(BitString *b);

    /* Sequential container specific functions */
    
    int (*Add)(BitString *BitStr,int);     /* Adds one bit at the end. */
    /* Returns the string at the given position */
    int (*GetElement)(BitString *BitStr,size_t idx);
    /* Pushes a bit */
    int        (*PushBack)(BitString *BitStr,int val);
    /* Pops the last bit off */
    int       (*PopBack)(BitString *BitStr);
    /* Inserts a string at the given position */
    size_t (*InsertAt)(BitString *BitStr,size_t idx,bool bit);
    /*erases the string at the indicated position */
    int (*EraseAt)(BitString *BitStr,size_t idx);
    /* Replaces the bit at the given position with a new one */
    int        (*ReplaceAt)(BitString *BitStr,size_t idx,bool newval);
    int (*IndexOf)(BitString *BitStr,bool SearchedBit,void *ExtraArgs,size_t *result);

    /* Bit string specific functions */
    
    /* Inserts a bit at the position zero. */
    size_t     (*Insert)(BitString *BitStr,bool bit);
    /* sets the given element to a new value */
    int        (*SetElement)(BitString *bs,size_t position,bool b);
    /* Returns the current capacity of the collection */
    size_t     (*GetCapacity)(BitString *BitStr);
    /* Sets the capacity if there are no items in the collection */
    int        (*SetCapacity)(BitString *BitStr,size_t newCapacity);
    /* Bit string specific functions */
    BitString *(*Or)(BitString *left,BitString *right);
    int        (*OrAssign)(BitString *bsl,BitString *bsr);
    BitString *(*And)(BitString *bsl,BitString *bsr);
    int        (*AndAssign)(BitString *bsl,BitString *bsr);
    BitString *(*Xor)(BitString *bsl,BitString *bsr);
    int        (*XorAssign)(BitString *bsl,BitString *bsr);
    BitString *(*Not)(BitString *bsl);
    int        (*NotAssign)(BitString *bsl);
    uintmax_t  (*PopulationCount)(BitString *b);
    uintmax_t  (*BitBlockCount)(BitString *b);
    int        (*LessEqual)(BitString *bsl,BitString *bsr);
    BitString *(*Reverse)(BitString *b);
    BitString *(*GetRange)(BitString *b,size_t start,size_t end);
    BitString *(*StringToBitString)(unsigned char *);
    BitString *(*ObjectToBitString)(size_t size,void *data);
    int        (*BitLeftShift)(BitString *bs,size_t shift);
    int        (*BitRightShift)(BitString *bs,size_t shift);
    size_t     (*Print)(BitString *b,size_t bufsiz,unsigned char *out);
    int        (*Append)(BitString *left,BitString *right);
    int        (*Memset)(BitString *,size_t start,size_t stop,bool newval);
    /* creates a bit string */
    BitString *(*Create)(size_t bitlen);
    BitString *(*Init)(BitString *BitStr,size_t bitlen);
    unsigned char *(*GetBits)(BitString *BitStr);
    int        (*CopyBits)(BitString *bitstr,void *buf);
    int (*AddRange)(BitString *b, size_t bitSize, void *data);
} BitStringInterface;

extern BitStringInterface iBitString;

/* -------------------------------------------------------------------------------------
 *                                                                                     *
 *                            Bloom filter                                             *
 *                                                                                     *
 * ----------------------------------------------------------------------------------  */
typedef struct tagBloomFilter BloomFilter;
typedef struct tagBloomFilterInterface {
    size_t (*CalculateSpace)(size_t nbOfElements,double Probability);
    BloomFilter *(*Create)(size_t MaxElements,double probability);
    size_t (*Add)(BloomFilter *b,const void *key,size_t keylen);
    int (*Find)(BloomFilter *b,const void *key,size_t keylen);
    int (*Clear)(BloomFilter *b);
    int (*Finalize)(BloomFilter *b);
} BloomFilterInterface;
extern BloomFilterInterface iBloomFilter;

/************************************************************************** */
/*                                                                          */
/*                          Observer interface                              */
/*                                                                          */
/************************************************************************** */
enum CCL_OPERATIONS{
    CCL_ADD=1,
    CCL_ADDRANGE = CCL_ADD << 1,
    CCL_ERASE_AT = CCL_ADDRANGE << 1,
    CCL_CLEAR = CCL_ERASE_AT<<1,
    CCL_FINALIZE = CCL_CLEAR<<1,
    CCL_PUSH = CCL_FINALIZE << 1,
    CCL_POP = CCL_PUSH << 1,
    CCL_REPLACE = CCL_POP << 1,
    CCL_REPLACEAT = CCL_REPLACE << 1,
    CCL_INSERT = CCL_REPLACEAT << 1,
    CCL_INSERT_AT = CCL_INSERT << 1,
    CCL_INSERT_IN = CCL_INSERT << 1,
    CCL_APPEND = CCL_INSERT_IN << 1,
    CCL_COPY = CCL_APPEND << 1
};
#define CCL_MODIFY (CCL_ADD|CCL_ERASE_AT|CCL_CLEAR|CCL_FINALIZE|CCL_POP| \
        CCL_PUSH|CCL_REPLACE|CCL_INSERT|CCL_APPEND|CCL_ADDRANGE)
#define CCL_ADDITIONS (CCL_ADD|CCL_PUSH|CCL_INSERT|CCL_APPEND|CCL_ADDRANGE)
#define CCL_DELETIONS (CCL_ERASE|CCL_CLEAR|CCL_FINALIZE|CCL_POP)
typedef void (*ObserverFunction)(void *ObservedObject,unsigned operation, void *ExtraInfo[]);

typedef struct tagObserverInterface {
    int (*Subscribe)(void *ObservedObject, ObserverFunction callback, unsigned Operations);
    int (*Notify)(void *ObservedObject,unsigned operation,void *ExtraInfo1,void *ExtraInfo2);
    size_t (*Unsubscribe)(void *ObservedObject,ObserverFunction callback);
} ObserverInterface;
extern ObserverInterface iObserver;
#endif
