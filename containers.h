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

/* General container flags */
#define CONTAINER_READONLY      1
#define CONTAINER_HAS_OBSERVER  2
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
#define CONTAINER_ERROR_BADARG          -15
#define CONTAINER_ERROR_NOMEMORY        -16
#define CONTAINER_ERROR_NOENT           -17
#define CONTAINER_ERROR_INCOMPATIBLE    -18
#define CONTAINER_ERROR_BADPOINTER      -19
#define CONTAINER_ERROR_BUFFEROVERFLOW  -20
#define CONTAINER_ERROR_DIVISION_BY_ZERO -21
#define CONTAINER_ERROR_WRONGELEMENT    -22
#define CONTAINER_ERROR_BADMASK         -23

typedef void *(*ErrorFunction)(const char *,int,...);
typedef int (*DestructorFunction)(void *);
typedef size_t (*HashFunction)(const char *); /* For the dictionary container */
typedef size_t (*WHashFunction)(const wchar_t *); /* For the dictionary container */

typedef struct tagError {
    ErrorFunction RaiseError;
    void *(*EmptyErrorFunction)(const char *fname,int errcode,...);
    char *(*StrError)(int errcode);
    ErrorFunction (*SetErrorFunction)(ErrorFunction);
    int (*NullPtrError)(const char *fname);
} ErrorInterface;

extern ErrorInterface iError;
/************************************************************************** */
/*                                                                          */
/*                          Memory allocation object                        */
/*                                                                          */
/************************************************************************** */
typedef struct tagAllocator {
    void *(*malloc)(size_t);        /* Function to allocate memory */
    void (*free)(void *);           /* Function to release it      */
    void *(*realloc)(void *,size_t);/* Function to resize a block of memory */
    void *(*calloc)(size_t,size_t);
} ContainerAllocator;
extern ContainerAllocator * CurrentAllocator;
ContainerAllocator *SetCurrentAllocator(ContainerAllocator *in);
extern ContainerAllocator iDebugMalloc;

/****************************************************************************
 *                   Masks interface                                        *
 ****************************************************************************/
typedef struct _Mask Mask;
typedef struct tagMaskInterface {
    int (*And)(Mask * src1,Mask * src2);
    int (*Or)(Mask * src1,Mask * src2);
    int (*Not)(Mask *src);
    Mask *(*CreateFromMask)(size_t n,char *data);
    Mask *(*Create)(size_t n);
    Mask *(*Copy)(Mask *src);
    size_t (*Size)(Mask *);
    size_t (*Sizeof)(Mask *);
    int (*Set)(Mask *m,size_t idx,int val);
    int (*Clear)(Mask *m);
    int (*Finalize)(Mask *m);
    size_t (*PopulationCount)(const Mask *m);
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
    size_t (*GetPosition)(struct _Iterator *);
    int   (*Replace)(struct _Iterator *,void *data,int direction);
} Iterator;

/* Type definition of the compare function */
typedef struct tagCompareInfo {
    void *ExtraArgs;
    const void *ContainerLeft;
    const void *ContainerRight;
} CompareInfo;
typedef int (*CompareFunction)(const void *elem1, const void *elem2,CompareInfo *ExtraArgs);
typedef int (*SaveFunction)(const void *element, void *arg, FILE *OutputStream);
typedef int (*ReadFunction)(void *element, void *arg, FILE *InputStream);

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
    ContainerHeap *(*Create)(size_t ElementSize,const ContainerAllocator *m);
    void *(*NewObject)(ContainerHeap *heap);
    int (*FreeObject)(ContainerHeap *heap,void *element);
    void (*Clear)(ContainerHeap *heap);
    void (*Finalize)(ContainerHeap *heap);
    ContainerHeap *(*InitHeap)(void *heap,size_t nbElements,const ContainerAllocator *allocator);
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
typedef struct tagPoolAllocatorInterface {
    Pool  *(*Create)(ContainerAllocator *m);
    void  *(*Alloc)(Pool *pool,size_t size);
    void  *(*Calloc)(Pool *pool,size_t n,size_t size);
    void   (*Clear)(Pool *);
    void   (*Finalize)(Pool *);
} PoolAllocatorInterface;

extern PoolAllocatorInterface iPool;

typedef struct tagPoolAllocatorDebugInterface {
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
    size_t (*Size)(const GenericContainer *Gen);
    unsigned (*GetFlags)(const GenericContainer *Gen);
    unsigned (*SetFlags)(GenericContainer *Gen,unsigned flags);
    int (*Clear)(GenericContainer *Gen);
    int (*Contains)(const GenericContainer *Gen,const void *Value);
    int (*Erase)(GenericContainer *Gen,const void *);
    int (*EraseAll)(GenericContainer *Gen,const void *);
    int (*Finalize)(GenericContainer *Gen);
    void (*Apply)(GenericContainer *Gen,int (*Applyfn)(void *,void * arg),void *arg);
    int (*Equal)(const GenericContainer * Gen1,const GenericContainer * Gen2);
    GenericContainer *(*Copy)(const GenericContainer *Gen);
    ErrorFunction (*SetErrorFunction)(GenericContainer *Gen,ErrorFunction fn);
    size_t (*Sizeof)(const GenericContainer *Gen);
    Iterator *(*NewIterator)(GenericContainer *Gen);
    int (*InitIterator)(GenericContainer *Gen,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const GenericContainer *Gen);
    int (*Save)(const GenericContainer *Gen,FILE *stream, SaveFunction saveFn,void *arg);
} GenericContainerInterface;
extern GenericContainerInterface iGeneric;

/************************************************************************** */
/*                                                                          */
/*                            Sequential Containers                         */
/*                                                                          */
/************************************************************************** */
typedef struct SequentialContainer SequentialContainer;
typedef struct tagSequentialContainerInterface {
    size_t (*Size)(const SequentialContainer *Gen);
    unsigned (*GetFlags)(const SequentialContainer *Gen);
    unsigned (*SetFlags)(SequentialContainer *Gen,unsigned flags);
    int (*Clear)(SequentialContainer *Gen);
    int (*Contains)(const SequentialContainer *Gen,const void *Value);
    int (*Erase)(SequentialContainer *Gen,const void *);
    int (*EraseAll)(SequentialContainer *Gen,const void *);
    int (*Finalize)(SequentialContainer *Gen);
    void (*Apply)(SequentialContainer *Gen,int (*Applyfn)(void *,void * arg),void *arg);
    int (*Equal)(const SequentialContainer *Gen1,const SequentialContainer *Gen2);
    SequentialContainer *(*Copy)(const SequentialContainer *Gen);
    ErrorFunction (*SetErrorFunction)(SequentialContainer *Gen,ErrorFunction fn);
    size_t (*Sizeof)(const SequentialContainer *Gen);
    Iterator *(*NewIterator)(SequentialContainer *Gen);
    int (*InitIterator)(SequentialContainer *Gen,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const SequentialContainer *Gen);
    int (*Save)(const SequentialContainer *Gen,FILE *stream, SaveFunction saveFn,void *arg);

    int (*Add)(SequentialContainer *SC,const void *Element);
    void *(*GetElement)(const SequentialContainer *SC,size_t idx);
    int (*Push)(SequentialContainer *Gen,void *Element);
    int (*Pop)(SequentialContainer *Gen,void *result);
    int (*InsertAt)(SequentialContainer *SC,size_t idx,const void *newval);
    int (*EraseAt)(SequentialContainer *SC,size_t idx);
    int (*ReplaceAt)(SequentialContainer *SC, size_t idx,const void *element);
    int (*IndexOf)(const SequentialContainer *SC,const void *ElementToFind,void *args,size_t *result);
    int (*Append)(SequentialContainer * SC1,SequentialContainer * SC2);
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
    int (*Add)(SequentialContainer *SC,const void *key,const void *Element);
    void *(*GetElement)(const AssociativeContainer *SC,const void *Key);
    int (*Replace)(AssociativeContainer *SC, const void *Key, const void *element);
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
                                         const ContainerAllocator *allocator);
    StreamBuffer *(*CreateFromFile)(const char *FileName);
    size_t (*Read)(StreamBuffer *b, void *data, size_t siz);
    size_t (*Write)(StreamBuffer *b,void *data, size_t siz);
    int (*SetPosition)(StreamBuffer *b,size_t pos);
    size_t (*GetPosition)(const StreamBuffer *b);
    char *(*GetData)(const StreamBuffer *b);
    size_t (*Size)(const StreamBuffer *b);
    int (*Clear)(StreamBuffer *b);
    int (*Finalize)(StreamBuffer *b);
    int (*Resize)(StreamBuffer *b,size_t newSize);
    int (*ReadFromFile)(StreamBuffer *b,FILE *f);
    int (*WriteToFile)(StreamBuffer *b,FILE *f);
} StreamBufferInterface;
extern StreamBufferInterface iStreamBuffer;
/************************************************************************** */
/*                                                                          */
/*                            circular buffers                              */
/*                                                                          */
/************************************************************************** */
typedef struct _CircularBuffer CircularBuffer;
typedef struct tagCircularBufferInterface {
    size_t (*Size)(const CircularBuffer *cb);
    int (*Add)( CircularBuffer * b, const void *data_element);
    int (*PopFront)(CircularBuffer *b,void *result);
    int (*PeekFront)(CircularBuffer *b,void *result);
    CircularBuffer *(*CreateWithAllocator)(size_t sizElement,
                  size_t sizeBuffer, const ContainerAllocator *allocator);
    CircularBuffer *(*Create)(size_t sizElement,size_t sizeBuffer);
    int (*Clear)(CircularBuffer *cb);
    int (*Finalize)(CircularBuffer *cb);
    size_t (*Sizeof)(const CircularBuffer *cb);
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

typedef int (*StringCompareFn)(const void **s1,const void **s2,CompareInfo *info);
typedef struct tagstrCollection {
/* -----------------------------------------------This is the generic container part */
    size_t (*Size)(const strCollection *SC);
    unsigned (*GetFlags)(const strCollection *SC);
    unsigned (*SetFlags)(strCollection *SC,unsigned flags);
    int (*Clear)(strCollection *SC);
    int (*Contains)(const strCollection *SC,const char *str);
    int (*Erase)(strCollection *SC,const char *);
    int (*EraseAll)(strCollection *SC,const char *);
    int (*Finalize)(strCollection *SC);
    int (*Apply)(strCollection *SC,int (*Applyfn)(char *,void * arg),void *arg);
    int (*Equal)(const strCollection *SC1,const strCollection *SC2);
    strCollection *(*Copy)(const strCollection *SC);
    ErrorFunction (*SetErrorFunction)(strCollection *SC,ErrorFunction fn);
    size_t (*Sizeof)(const strCollection *SC);
    Iterator *(*NewIterator)(strCollection *SC);
    int (*InitIterator)(strCollection *SC,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const strCollection *l);
    int (*Save)(const strCollection *SC,FILE *stream, SaveFunction saveFn,void *arg);
    strCollection *(*Load)(FILE *stream, ReadFunction readFn,void *arg);
    size_t (*GetElementSize)(const strCollection *SC);
 /* -------------------------------------------This is the Sequential container part */
    int (*Add)(strCollection *SC,const char *newval);
    int (*PushFront)(strCollection *SC,char *str);
    size_t (*PopFront)(strCollection *SC,char *outbuf,size_t buflen);
    int (*InsertAt)(strCollection *SC,size_t idx,const char *newval);
    int (*EraseAt)(strCollection *SC,size_t idx);
    int (*ReplaceAt)(strCollection *SC,size_t idx,char *newval);
    int (*IndexOf)(const strCollection *SC,const char *SearchedString,size_t *result);   
 /* ---------------------------------------------This is the specific container part */
    int (*Sort)(strCollection *SC);
    struct _Vector *(*CastToArray)(const strCollection *SC);
    size_t (*FindFirst)(const strCollection *SC,const char *text);
    size_t (*FindNext)(const strCollection *SC, const char *text,size_t start);
    strCollection *(*FindText)(const strCollection *SC,const char *text);
    Vector *(*FindTextIndex)(const strCollection *SC,const char *text);
    Vector *(*FindTextPositions)(const strCollection *SC,const char *text);
    int (*WriteToFile)(const strCollection *SC,const char *filename);
    strCollection *(*IndexIn)(const strCollection *SC,const Vector *AL);
    strCollection *(*CreateFromFile)(const char *fileName);
    strCollection *(*Create)(size_t startsize);
    strCollection *(*CreateWithAllocator)(size_t startsiz,const ContainerAllocator *mm);

    int (*AddRange)(strCollection *s,size_t n,const char **values);
    char **(*CopyTo)(const strCollection *SC);

    int (*Insert)(strCollection *SC,char *);
    int (*InsertIn)(strCollection *source, size_t idx,strCollection *newData);
    char *(*GetElement)(const strCollection *SC,size_t idx);
    size_t (*GetCapacity)(const strCollection *SC);
    int (*SetCapacity)(strCollection *SC,size_t newCapacity);
    StringCompareFn (*SetCompareFunction)(strCollection *SC,StringCompareFn);
    int (*Reverse)(strCollection *SC);
    int (*Append)(strCollection *,strCollection *);
    size_t (*PopBack)(strCollection *, char *result,size_t bufsize);
    int (*PushBack)(strCollection *,const char *data);
    strCollection *(*GetRange)(strCollection *SC, size_t start,size_t end);
    const ContainerAllocator *(*GetAllocator)(const strCollection *AL);
    int (*Mismatch)(const strCollection *a1,const strCollection *a2,size_t *mismatch);
    strCollection *(*InitWithAllocator)(strCollection *c,size_t start,const ContainerAllocator *mm);
    strCollection *(*Init)(strCollection *result,size_t startsize);
    DestructorFunction (*SetDestructor)(strCollection *v,DestructorFunction fn);
    strCollection *(*InitializeWith)(size_t n, char **data);
    char **(*GetData)(const strCollection *SC);
    char *(*Back)(const strCollection *str);
    char *(*Front)(const strCollection *str);
    int (*RemoveRange)(strCollection *SC,size_t start,size_t end);
    Mask *(*CompareEqual)(const strCollection *left, const strCollection *right,Mask *m);
    Mask *(*CompareEqualScalar)(const strCollection *left, const char *str,Mask *m);
    int (*Select)(strCollection *src, const Mask *m);
    strCollection *(*SelectCopy)(const strCollection *src,const Mask *m);
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
    size_t (*Size)(const WstrCollection *SC);
    unsigned (*GetFlags)(const WstrCollection *SC);
    unsigned (*SetFlags)(WstrCollection *SC,unsigned flags);
    int (*Clear)(WstrCollection *SC);
    int (*Contains)(const WstrCollection *SC,const wchar_t *str);
    int (*Erase)(WstrCollection *SC,const wchar_t *);
    int (*EraseAll)(WstrCollection *SC,const wchar_t *str);
    int (*Finalize)(WstrCollection *SC);
    int (*Apply)(WstrCollection *SC,int (*Applyfn)(wchar_t *,void * arg),void *arg);
    int (*Equal)(const WstrCollection *SC1,const WstrCollection *SC2);
    WstrCollection *(*Copy)(const WstrCollection *SC);
    ErrorFunction (*SetErrorFunction)(WstrCollection *SC,ErrorFunction fn);
    size_t (*Sizeof)(const WstrCollection *SC);
    Iterator *(*NewIterator)(WstrCollection *SC);
    int (*InitIterator)(WstrCollection *SC,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const WstrCollection *);
    int (*Save)(const WstrCollection *SC,FILE *stream, SaveFunction saveFn,void *arg);
    WstrCollection *(*Load)(FILE *stream, ReadFunction readFn,void *arg);
    size_t (*GetElementSize)(const WstrCollection *SC);
    /* -------------------------------------------This is the Sequential container part */
    int (*Add)(WstrCollection *SC,const wchar_t *newval);
    int (*PushFront)(WstrCollection *SC,wchar_t *str);
    size_t (*PopFront)(WstrCollection *SC,wchar_t *outbuf,size_t buflen);
    int (*InsertAt)(WstrCollection *SC,size_t idx,const wchar_t *newval);
    int (*EraseAt)(WstrCollection *SC,size_t idx);
    int (*ReplaceAt)(WstrCollection *SC,size_t idx,wchar_t *newval);
    int (*IndexOf)(const WstrCollection *SC,const wchar_t *SearchedString,size_t *result);
    /* ---------------------------------------------This is the specific container part */
    int (*Sort)(WstrCollection *SC);
    struct _Vector *(*CastToArray)(const WstrCollection *SC);
    size_t (*FindFirst)(const WstrCollection *SC,const wchar_t *text);
    size_t (*FindNext)(const WstrCollection *SC, const wchar_t *text,size_t start);
    WstrCollection *(*FindText)(const WstrCollection *SC,const wchar_t *text);
    Vector *(*FindTextIndex)(const WstrCollection *SC,const wchar_t *text);
    Vector *(*FindTextPositions)(const WstrCollection *SC,const wchar_t *text);
    int (*WriteToFile)(const WstrCollection *SC, const char *filename);
    WstrCollection *(*IndexIn)(const WstrCollection *SC,const Vector *AL);
    WstrCollection *(*CreateFromFile)(const char *fileName);
    WstrCollection *(*Create)(size_t startsize);
    WstrCollection *(*CreateWithAllocator)(size_t startsize,const ContainerAllocator *mm);
    
    int (*AddRange)(WstrCollection *s,size_t n,const wchar_t **values);
    wchar_t **(*CopyTo)(const WstrCollection *SC);
    
    int (*Insert)(WstrCollection *SC,wchar_t *);
    int (*InsertIn)(WstrCollection *source, size_t idx,WstrCollection *newData);
    wchar_t *(*GetElement)(const WstrCollection *SC,size_t idx);
    size_t (*GetCapacity)(const WstrCollection *SC);
    int (*SetCapacity)(WstrCollection *SC,size_t newCapacity);
    StringCompareFn (*SetCompareFunction)(WstrCollection *SC,StringCompareFn);
    int (*Reverse)(WstrCollection *SC);
    int (*Append)(WstrCollection *,WstrCollection *);
    size_t (*PopBack)(WstrCollection *,wchar_t *result,size_t bufsize);
    int (*PushBack)(WstrCollection *,const wchar_t *data);
    WstrCollection *(*GetRange)(WstrCollection *SC, size_t start,size_t end);
    const ContainerAllocator *(*GetAllocator)(const WstrCollection *AL);
    int (*Mismatch)(const WstrCollection *a1,const WstrCollection *a2,size_t *mismatch);
    WstrCollection *(*InitWithAllocator)(WstrCollection *result,size_t startsize,const ContainerAllocator *mm);
    WstrCollection *(*Init)(WstrCollection *result,size_t startsize);
    DestructorFunction (*SetDestructor)(WstrCollection *v,DestructorFunction fn);
    WstrCollection *(*InitializeWith)(size_t n,wchar_t **data);
    wchar_t **(*GetData)(const WstrCollection *SC);
    wchar_t *(*Back)(const WstrCollection *SC);
    wchar_t *(*Front)(const WstrCollection *SC);
    int (*RemoveRange)(WstrCollection *SC,size_t start,size_t end); 
    Mask *(*CompareEqual)(const WstrCollection *left, const WstrCollection *right,Mask *m);
    Mask *(*CompareEqualScalar)(const WstrCollection *left, const wchar_t *str,Mask *m);
    int (*Select)(WstrCollection *src,const Mask*m);
    WstrCollection *(*SelectCopy)(const WstrCollection *src, const Mask *m);
//    wchar_t *Find(WstrCollection *SC,wchar_t *data,CompareInfo *ci);
} WstrCollectionInterface;

extern WstrCollectionInterface iWstrCollection;

/* -------------------------------------------------------------------------
 * LIST Interface                                                           *
 * This describes the single linked list data structure functions          *
 * Each list objects has a list composed of list elements.                 *
 *-------------------------------------------------------------------------*/
typedef struct _List List;
typedef struct _ListElement ListElement;
typedef struct tagList {
    size_t (*Size)(const List *L);
    unsigned (*GetFlags)(const List *L);
    unsigned (*SetFlags)(List *L,unsigned flags);
    int (*Clear)(List *L);
    int (*Contains)(const List *L,const void *element);
    int (*Erase)(List *L,const void *);
    int (*EraseAll)(List *l,const void *);
    int (*Finalize)(List *L);
    int (*Apply)(List *L,int(Applyfn)(void *,void *),void *arg);
    int (*Equal)(const List *l1,const List *l2);
    List *(*Copy)(const List *L);
    ErrorFunction (*SetErrorFunction)(List *L,ErrorFunction);
    size_t (*Sizeof)(const List *l);
    Iterator *(*NewIterator)(List *L);
    int (*InitIterator)(List *L,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const List *);
    int (*Save)(const List *L,FILE *stream, SaveFunction saveFn,void *arg);
    List *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    size_t (*GetElementSize)(const List *l);
 /* -------------------------------------------This is the Sequential container part */
    int (*Add)(List *L,const void *newval);
    void *(*GetElement)(const List *L,size_t idx);
    int (*PushFront)(List *L,const void *str);
    int (*PopFront)(List *L,void *result);
    int (*InsertAt)(List *L,size_t idx,const void *newval);
    int (*EraseAt)(List *L,size_t idx);
    int (*ReplaceAt)(List *L,size_t idx,const void *newval);
    int (*IndexOf)(const List *L,const void *SearchedElement,void *ExtraArgs,size_t *result);
/* -------------------------------------------This is the list container part */
    int (*InsertIn)(List *l, size_t idx,List *newData);
    int  (*CopyElement)(const List *list,size_t idx,void *OutBuffer);
    int (*EraseRange)(List *L,size_t start,size_t end);
    int (*Sort)(List *l);
    int (*Reverse)(List *l);
    List *(*GetRange)(const List *l,size_t start,size_t end);
    int (*Append)(List *l1,List *l2);
    CompareFunction (*SetCompareFunction)(List *l,CompareFunction fn);
    CompareFunction Compare;
    int (*UseHeap)(List *L, const ContainerAllocator *m);
    ContainerHeap *(*GetHeap)(const List *l);
    int (*AddRange)(List *L, size_t n,const void *data);
    List *(*Create)(size_t element_size);
    List *(*CreateWithAllocator)(size_t elementsize,const ContainerAllocator *mm);
    List *(*Init)(List *aList,size_t element_size);
    List *(*InitWithAllocator)(List *aList,size_t element_size,const ContainerAllocator *mm);
    const ContainerAllocator *(*GetAllocator)(const List *list);
    DestructorFunction (*SetDestructor)(List *v,DestructorFunction fn);
    List *(*InitializeWith)(size_t elementSize,size_t n,const void *data);
    void *(*Back)(const List *l);
    void *(*Front)(const List *l);
    int (*RemoveRange)(List *l,size_t start, size_t end);
    int (*RotateLeft)(List *l, size_t n);
    int (*RotateRight)(List *l,size_t n);
    int (*Select)(List *src,const Mask *m);
    List *(*SelectCopy)(const List *src,const Mask *m);
    ListElement *(*FirstElement)(List *l);
    ListElement *(*LastElement)(List *l);
    ListElement *(*NextElement)(ListElement *le);
    void *(*ElementData)(ListElement *le);
    int (*SetElementData)(List *l, ListElement *le,void *data);
    void *(*Advance)(ListElement **pListElement);
    ListElement *(*Skip)(ListElement *l,size_t n);
    List *(*SplitAfter)(List *l, ListElement *pt);
} ListInterface;

extern ListInterface iList;

#include "stringlist.h"
#include "wstringlist.h"
/* -------------------------------------------------------------------
 *                           QUEUES
 * -------------------------------------------------------------------*/
typedef struct _Queue Queue;

typedef struct tagQueueInterface {
    Queue *(*Create)(size_t elementSize);
    Queue *(*CreateWithAllocator)(size_t elementSize, ContainerAllocator *allocator);
    size_t (*Size)(Queue *Q);
    size_t (*Sizeof)(Queue *Q);
    int (*Enqueue)(Queue *Q, void *Element);
    int (*Dequeue)(Queue *Q,void *result);
    int (*Clear)(Queue *Q);
    int (*Finalize)(Queue *Q);
    int (*Front)(Queue *Q,void *result);
    int (*Back)(Queue *Q,void *result);
    List *(*GetData)(Queue *q);
} QueueInterface;

extern QueueInterface iQueue;
/* -------------------------------------------------------------------
 *                      PRIORITY  QUEUES
 * -------------------------------------------------------------------*/
typedef struct _PQueue PQueue;
typedef struct _PQueueElement PQueueElement;

typedef struct tagPQueueInterface {
    int (*Add)(PQueue *Q,intptr_t key,const void *Element);
    size_t (*Size)(const PQueue *Q);
    PQueue *(*Create)(size_t elementSize);
    PQueue *(*CreateWithAllocator)(size_t elementSize, ContainerAllocator *allocator);
    int (*Equal)(const PQueue *q1,const PQueue *q2);
    size_t (*Sizeof)(const PQueue *Q);
    int (*Push)(PQueue *Q,intptr_t key,const void *Element);
    int (*Clear)(PQueue *Q);
    int (*Finalize)(PQueue *Q);
    intptr_t (*Pop)(PQueue *Q,void *result);
    intptr_t (*Front)(const PQueue *Q,void *result);
    PQueue *(*Copy)(const PQueue *src);
    PQueue *(*Union)(PQueue *left, PQueue *right);
//  int (*Replace)(PQueue *src,intptr_t key,void *data);
} PQueueInterface;

extern PQueueInterface iPQueue;

/* --------------------------------------------------------------------
 *                           Double Ended QUEues                      *
 * -------------------------------------------------------------------*/
typedef struct deque_t Deque;
typedef struct tagDequeInterface {
    size_t (*Size)(Deque *Q);
    unsigned (*GetFlags)(Deque *Q);
    unsigned (*SetFlags)(Deque *Q,unsigned newFlags);
    int (*Clear)(Deque *Q);
    size_t (*Contains)(Deque * d, void* item);
    int (*Erase)(Deque * d, const void* item);
    int (*EraseAll)(Deque * d, const void* item);
    int (*Finalize)(Deque *Q);
    void (*Apply)(Deque *Q,int (*Applyfn)(void *,void * arg),void *arg);
    int (*Equal)(Deque *d1,Deque *d2);
    Deque *(*Copy)(Deque *d);
    ErrorFunction (*SetErrorFunction)(Deque *d,ErrorFunction); 
    size_t (*Sizeof)(Deque *d);
    Iterator *(*NewIterator)(Deque *Deq);
    int (*InitIterator)(Deque *dc,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(Deque *);
    int (*Save)(const Deque *d,FILE *stream, SaveFunction saveFn,void *arg);
    Deque *(*Load)(FILE *stream, ReadFunction readFn,void *arg);
    /* Deque specific functions */
    int (*PushBack)(Deque *Q,const void *Element);
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
typedef struct _DlistElement DlistElement;
typedef struct tagDlist {
    size_t (*Size)(const Dlist *dl);
    unsigned (*GetFlags)(const Dlist *AL);
    unsigned (*SetFlags)(Dlist *AL,unsigned flags);
    int (*Clear)(Dlist *dl);       
    int (*Contains)(const Dlist *dl,const void *element);
    int (*Erase)(Dlist *AL,const void *);
    int (*EraseAll)(Dlist *AL,const void *);
    int (*Finalize)(Dlist *AL);
    int (*Apply)(Dlist *L,int(Applyfn)(void *elem,void *extraArg),void *extraArg);
    int (*Equal)(const Dlist *l1,const Dlist *l2); 
    Dlist *(*Copy)(const Dlist *dl);
    ErrorFunction (*SetErrorFunction)(Dlist *L,ErrorFunction);
    size_t (*Sizeof)(const Dlist *dl);
    Iterator *(*NewIterator)(Dlist *);
    int (*InitIterator)(Dlist *,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const Dlist *);
    int (*Save)(const Dlist *L,FILE *stream, SaveFunction saveFn,void *arg);
    Dlist *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    size_t (*GetElementSize)(const Dlist *dl);

    /* -----------------------------------------Sequential container part */
    int (*Add)(Dlist *dl,const void *newval);
    void *(*GetElement)(const Dlist *AL,size_t idx);
    int (*PushFront)(Dlist *AL,const void *str); 
    int (*PopFront)(Dlist *AL,void *result);
    int (*InsertAt)(Dlist *AL,size_t idx,const void *newval);
    int (*EraseAt)(Dlist *AL,size_t idx); 
    int (*ReplaceAt)(Dlist *AL,size_t idx,const void *newval); 
    int (*IndexOf)(const Dlist *AL,const void *SearchedElement,void *args,size_t *result); 

    /* ------------------------------------Dlist container specific part */
    int (*PushBack)(Dlist *AL,const void *str);  
    int (*PopBack)(Dlist *AL,void *result);
    Dlist *(*Splice)(Dlist *list,void *pos,Dlist *toInsert,int direction);
    int (*Sort)(Dlist *l);
    int (*Reverse)(Dlist *l);
    Dlist *(*GetRange)(Dlist *l,size_t start,size_t end);
    int (*Append)(Dlist *l1,Dlist *l2);
    CompareFunction (*SetCompareFunction)(Dlist *l,CompareFunction fn);
    int (*UseHeap)(Dlist *L,const ContainerAllocator *m);
    int (*AddRange)(Dlist *l,size_t n,const void *data);
    Dlist *(*Create)(size_t elementsize);
    Dlist *(*CreateWithAllocator)(size_t,const ContainerAllocator *);
    Dlist *(*Init)(Dlist *dlist,size_t elementsize);
    Dlist *(*InitWithAllocator)(Dlist *L,size_t element_size,const ContainerAllocator *mm);
    int (*CopyElement)(const Dlist *l,size_t idx,void *outbuf);
    int (*InsertIn)(Dlist *l, size_t idx,Dlist *newData);
    DestructorFunction (*SetDestructor)(Dlist *v,DestructorFunction fn);
    Dlist *(*InitializeWith)(size_t elementSize, size_t n,const void *data);
    const ContainerAllocator *(*GetAllocator)(const Dlist *l);
    void *(*Back)(const Dlist *l);
    void *(*Front)(const Dlist *l);
    int (*RemoveRange)(Dlist *l,size_t start, size_t end);
    int (*RotateLeft)(Dlist *l, size_t n);
    int (*RotateRight)(Dlist *l,size_t n);
    int (*Select)(Dlist *src,const Mask *m);
    Dlist *(*SelectCopy)(const Dlist *src,const Mask *m);
    DlistElement *(*FirstElement)(Dlist *l);
    DlistElement *(*LastElement)(Dlist *l);
    DlistElement *(*NextElement)(DlistElement *le);
    DlistElement *(*PreviousElement)(DlistElement *le);
    void *(*ElementData)(DlistElement *le);
    int (*SetElementData)(Dlist *l, DlistElement *le,void *data);
    void *(*Advance)(DlistElement **pDlistElement);
    DlistElement *(*Skip)(DlistElement *l,size_t n);
    void *(*MoveBack)(DlistElement **pDlistElement);
    Dlist *(*SplitAfter)(Dlist *l, DlistElement *pt);
} DlistInterface;

extern DlistInterface iDlist;

/****************************************************************************
 *           Vectors                                                        *
 ****************************************************************************/

/* Definition of the functions associated with this type. */
typedef struct tagVector {
    size_t (*Size)(const Vector *AL);
    unsigned (*GetFlags)(const Vector *AL);
    unsigned (*SetFlags)(Vector *AL,unsigned flags);
    int (*Clear)(Vector *AL);
    /*Case sensitive search of a character string in the data */
    int (*Contains)(const Vector *AL,const void *element,void *ExtraArgs);
    int (*Erase)(Vector *AL,const void *);
    int (*EraseAll)(Vector *AL,const void *);
    int (*Finalize)(Vector *AL);
    int (*Apply)(Vector *AL,int (*Applyfn)(void *element,void * arg),void *arg);
    int (*Equal)(const Vector *first,const Vector *second);
    Vector *(*Copy)(const Vector *AL);
    ErrorFunction (*SetErrorFunction)(Vector *AL,ErrorFunction);
    size_t (*Sizeof)(const Vector *AL);
    Iterator *(*NewIterator)(Vector *AL);
    int (*InitIterator)(Vector *V,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const Vector *);
    int (*Save)(const Vector *AL,FILE *stream, SaveFunction saveFn,void *arg);
    Vector *(*Load)(FILE *stream, ReadFunction readFn,void *arg);
    size_t (*GetElementSize)(const Vector *AL);

    /* Sequential container specific fields */

    int (*Add)(Vector *AL,const void *newval);
    void *(*GetElement)(const Vector *AL,size_t idx);
    int (*PushBack)(Vector *AL,const void *str);
    int (*PopBack)(Vector *AL,void *result);
    int (*InsertAt)(Vector *AL,size_t idx,void *newval);
    int (*EraseAt)(Vector *AL,size_t idx);
    int (*ReplaceAt)(Vector *AL,size_t idx,void *newval);
    int (*IndexOf)(const Vector *AL,const void *data,void *ExtraArgs,size_t *result);

    /* Vector container specific fields */

    int (*Insert)(Vector *AL,void *);
    int (*InsertIn)(Vector *AL, size_t idx,Vector *newData);
    Vector *(*IndexIn)(Vector *SC,Vector *AL);
    size_t (*GetCapacity)(const Vector *AL);
    int (*SetCapacity)(Vector *AL,size_t newCapacity);

    CompareFunction (*SetCompareFunction)(Vector *l,CompareFunction fn);
    int (*Sort)(Vector *AL);
    Vector *(*Create)(size_t elementsize,size_t startsize);
    Vector *(*CreateWithAllocator)(size_t elemsiz,size_t startsiz,const ContainerAllocator *mm);
    Vector *(*Init)(Vector *r,size_t elementsize,size_t startsize);
    int (*AddRange)(Vector *AL,size_t n,const void *newvalues);
    Vector *(*GetRange)(const Vector *AL, size_t start, size_t end);
    int (*CopyElement)(const Vector *AL,size_t idx,void *outbuf);
    void **(*CopyTo)(const Vector *AL);
    int (*Reverse)(Vector *AL);
    int (*Append)(Vector *AL1, Vector *AL2);
    int (*Mismatch)(Vector *a1,Vector *a2,size_t *mismatch);
    const ContainerAllocator *(*GetAllocator)(const Vector *AL);
    DestructorFunction (*SetDestructor)(Vector *v,DestructorFunction fn);
    int (*SearchWithKey)(Vector *vec,size_t startByte,size_t sizeKey,size_t startIndex,void *item,size_t *result);
    int (*Select)(Vector *src,const Mask *m);
    Vector *(*SelectCopy)(Vector *src,Mask *m);
    int (*Resize)(Vector *AL,size_t newSize);
    Vector *(*InitializeWith)(size_t elementSize, size_t n,const void *Data);
    void **(*GetData)(const Vector *AL);
    void *(*Back)(const Vector *AL);
    void *(*Front)(const Vector *AL);
    int (*RemoveRange)(Vector *SC,size_t start,size_t end);
    int (*RotateLeft)(Vector *V,size_t n);
    int (*RotateRight)(Vector *V,size_t n);
    Mask *(*CompareEqual)(const Vector *left,const Vector *right,Mask *m);
    Mask *(*CompareEqualScalar)(const Vector *left, const void *right,Mask *m);
    int (*Reserve)(Vector *src,size_t newCapacity);
} VectorInterface;

extern VectorInterface iVector;

#include "valarray.h"
typedef struct _Dictionary Dictionary;
typedef struct tagDictionary {
    size_t (*Size)(const Dictionary *Dict);
    unsigned (*GetFlags)(const Dictionary *Dict);
    unsigned (*SetFlags)(Dictionary *Dict,unsigned flags);
    int (*Clear)(Dictionary *Dict);
    int (*Contains)(const Dictionary *dict,const char *key);
    int (*Erase)(Dictionary *Dict,const char *);
    int (*Finalize)(Dictionary *Dict);
    int (*Apply)(Dictionary *Dict,int (*Applyfn)(const char *Key,
                                                  const void *data,void *arg),void *arg);
    int (*Equal)(const Dictionary *d1,const Dictionary *d2);
    Dictionary *(*Copy)(const Dictionary *dict);
    ErrorFunction (*SetErrorFunction)(Dictionary *Dict,ErrorFunction fn);
    size_t (*Sizeof)(const Dictionary *dict);
    Iterator *(*NewIterator)(Dictionary *dict);
    int (*InitIterator)(Dictionary *dict,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const Dictionary *);
    int (*Save)(const Dictionary *Dict,FILE *stream, SaveFunction saveFn,void *arg);
    Dictionary * (*Load)(FILE *stream, ReadFunction readFn, void *arg);
    size_t (*GetElementSize)(const Dictionary *d);

    /* Hierarchical container specific fields */

    int (*Add)(Dictionary *Dict,const char *key,const void *Data);
    void *(*GetElement)(const Dictionary *Dict,const char *Key);
    int (*Replace)(Dictionary *dict,const char *Key,const void *Data);
    int (*Insert)(Dictionary *Dict,const char *key,const void *Data);

    /* Dictionary specific fields */

    Vector *(*CastToArray)(const Dictionary *);
    int (*CopyElement)(const Dictionary *Dict,const char *Key,void *outbuf);
    int (*InsertIn)(Dictionary *dst,Dictionary *src);
    Dictionary *(*Create)(size_t ElementSize,size_t hint);
    Dictionary *(*CreateWithAllocator)(size_t elementsize,size_t hint,const ContainerAllocator *mm);
    Dictionary *(*Init)(Dictionary *dict,size_t ElementSize,size_t hint);
    Dictionary *(*InitWithAllocator)(Dictionary *D,size_t elemsize,size_t hint,const ContainerAllocator *mm);
    strCollection *(*GetKeys)(const Dictionary *Dict);
    const ContainerAllocator *(*GetAllocator)(const Dictionary *Dict);
    DestructorFunction (*SetDestructor)(Dictionary *v,DestructorFunction fn);
    Dictionary *(*InitializeWith)(size_t elementSize,size_t n, const char **Keys,const void *Values);
    HashFunction (*SetHashFunction)(Dictionary *d,HashFunction newFn);
    double (*GetLoadFactor)(Dictionary *d);
} DictionaryInterface;

extern DictionaryInterface iDictionary;

typedef struct _WDictionary WDictionary;
typedef struct tagWDictionary {
    size_t (*Size)(const WDictionary *Dict);
    unsigned (*GetFlags)(const WDictionary *Dict);
    unsigned (*SetFlags)(WDictionary *Dict,unsigned flags);
    int (*Clear)(WDictionary *Dict);
    int (*Contains)(const WDictionary *dict,const wchar_t *key);
    int (*Erase)(WDictionary *Dict,const wchar_t *);
    int (*Finalize)(WDictionary *Dict);
    int (*Apply)(WDictionary *Dict,int (*Applyfn)(const wchar_t *Key,
                                                  const void *data,void *arg),void *arg);
    int (*Equal)(const WDictionary *d1,const WDictionary *d2);
    WDictionary *(*Copy)(const WDictionary *dict);
    ErrorFunction (*SetErrorFunction)(WDictionary *Dict,ErrorFunction fn);
    size_t (*Sizeof)(const WDictionary *dict);
    Iterator *(*NewIterator)(WDictionary *dict);
    int (*InitIterator)(WDictionary *dict,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const WDictionary *);
    int (*Save)(const WDictionary *Dict,FILE *stream, SaveFunction saveFn,void *arg);
    WDictionary * (*Load)(FILE *stream, ReadFunction readFn, void *arg);
    size_t (*GetElementSize)(const WDictionary *d);

    /* Hierarchical container specific fields */

    int (*Add)(WDictionary *Dict,const wchar_t *key,const void *Data);
    void *(*GetElement)(const WDictionary *Dict,const wchar_t *Key);
    int (*Replace)(WDictionary *dict,const wchar_t *Key,const void *Data);
    int (*Insert)(WDictionary *Dict,const wchar_t *key,const void *Data);

    /* WDictionary specific fields */

    Vector *(*CastToArray)(const WDictionary *);
    int (*CopyElement)(const WDictionary *Dict,const wchar_t *Key,void *outbuf);
    int (*InsertIn)(WDictionary *dst,WDictionary *src);
    WDictionary *(*Create)(size_t ElementSize,size_t hint);
    WDictionary *(*CreateWithAllocator)(size_t elementsize,size_t hint,const ContainerAllocator *mm);
    WDictionary *(*Init)(WDictionary *dict,size_t ElementSize,size_t hint);
    WDictionary *(*InitWithAllocator)(WDictionary *D,size_t elemsize,size_t hint,const ContainerAllocator *mm);
    WstrCollection *(*GetKeys)(const WDictionary *Dict);
    const ContainerAllocator *(*GetAllocator)(const WDictionary *Dict);
    DestructorFunction (*SetDestructor)(WDictionary *v,DestructorFunction fn);
    WDictionary *(*InitializeWith)(size_t elementSize,size_t n, const wchar_t **Keys,const void *Values);
    WHashFunction (*SetHashFunction)(WDictionary *d,WHashFunction newFn);
    double (*GetLoadFactor)(WDictionary *d);
} WDictionaryInterface;
extern WDictionaryInterface iWDictionary;

/* ----------------------------------Hash table interface -----------------------*/
typedef struct _HashTable HashTable;
typedef unsigned int (*GeneralHashFunction)(const char *char_key, size_t *klen);

typedef struct tagHashTable {
    size_t (*Size)(const HashTable *HT);
    unsigned (*GetFlags)(const HashTable *HT);
    unsigned (*SetFlags)(HashTable *HT,unsigned flags);
    int (*Clear)(HashTable *HT);
    int (*Contains)(const HashTable *ht,const void *Key,size_t klen);
    HashTable *(*Create)(size_t ElementSize);
    HashTable *(*Init)(HashTable *ht,size_t ElementSize);
    size_t (*Sizeof)(const HashTable *HT);
    size_t (*GetElementSize)(const HashTable *HT);
    int (*Add)(HashTable *HT,const void *key,size_t klen,const void *Data);
    void *(*GetElement)(const HashTable *HT,const void *Key,size_t klen);
    int (*Search)(HashTable *ht,int (*Comparefn)(void *rec, const void *key,size_t klen,const void *value), void *rec);
    int (*Erase)(HashTable *HT,const void *key,size_t klen);
    int (*Finalize)(HashTable *HT);
    int (*Apply)(HashTable *HT,int (*Applyfn)(void *Key,size_t klen,void *data,void *arg),void *arg);
    ErrorFunction (*SetErrorFunction)(HashTable *HT,ErrorFunction fn);
    int (*Resize)(HashTable *HT,size_t newSize);
    int (*Replace)(HashTable *HT,const void *key, size_t klen,const void *val);
    HashTable *(*Copy)(const HashTable *Orig,Pool *pool);
    GeneralHashFunction (*SetHashFunction)(HashTable *ht, GeneralHashFunction hf);
    HashTable *(*Overlay)(Pool *p, const HashTable *overlay, const HashTable *base);
    HashTable *(*Merge)(Pool *p, const HashTable *overlay, const HashTable *base, void * (*merger)(Pool *p, const void *key, size_t klen, const void *h1_val, const void *h2_val, const void *data), const void *data);
    Iterator *(*NewIterator)(HashTable *);
    int (*InitIterator)(HashTable *SC,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const HashTable *ht);
    int (*Save)(const HashTable *HT,FILE *stream, SaveFunction saveFn,void *arg);
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
    size_t (*Size)(BinarySearchTree *ST);
    unsigned (*GetFlags)(BinarySearchTree *ST);
    unsigned (*SetFlags)(BinarySearchTree *ST, unsigned flags);
    int (*Clear)(BinarySearchTree *ST);
    int (*Contains)(BinarySearchTree *ST,void *data);
    int (*Erase)(BinarySearchTree *ST, const void *,void *);
    int (*Finalize)(BinarySearchTree *ST);
    int (*Apply)(BinarySearchTree *ST,int (*Applyfn)(const void *data,void *arg),void *arg);
    int (*Equal)(const BinarySearchTree *left, const BinarySearchTree *right);
    int (*Add)(BinarySearchTree *ST, const void *Data);
    int (*Insert)(BinarySearchTree * ST, const void *Data, void *ExtraArgs);
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
    int (*Add)(RedBlackTree *ST, const void *Key,const void *Data);
    int (*Insert)(RedBlackTree *RB, const void *Key, const void *Data, void *ExtraArgs);
    int (*Clear)(RedBlackTree *ST);
    int (*Erase)(RedBlackTree *ST, const void *,void *);
    int (*Finalize)(RedBlackTree *ST);
    int (*Apply)(RedBlackTree *ST,int (*Applyfn)(const void *data,void *arg),void *arg);
    void *(*Find)(RedBlackTree *ST,void *key_data,void *ExtraArgs);
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
    int (*Erase)(TreeMap *tree, const void *element,void *ExtraArgs);  /* erases the given data if found */
    int (*Finalize)(TreeMap *ST);  /* Frees the memory used by the tree */
    int (*Apply)(TreeMap *ST,int (*Applyfn)(const void *data,void *arg),void *arg);
    int (*Equal)(TreeMap *t1, TreeMap *t2);
    TreeMap *(*Copy)(TreeMap *src);
    ErrorFunction (*SetErrorFunction)(TreeMap *ST, ErrorFunction fn);
    size_t (*Sizeof)(TreeMap *ST);
    Iterator *(*NewIterator)(TreeMap *);
    int (*InitIterator)(TreeMap *,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(TreeMap *);
    int (*Save)(const TreeMap *src,FILE *stream, SaveFunction saveFn,void *arg);
    int (*Add)(TreeMap *ST, void *Data,void *ExtraArgs); /* Adds one element. Given data is copied */
    int (*AddRange)(TreeMap *ST,size_t n, void *Data,void *ExtraArgs);
    int (*Insert)(TreeMap *RB, const void *Data, void *ExtraArgs);
    void *(*GetElement)(TreeMap *tree,const void *element,void *ExtraArgs);
    CompareFunction (*SetCompareFunction)(TreeMap *ST,CompareFunction fn);
    TreeMap *(*CreateWithAllocator)(size_t ElementSize,const ContainerAllocator *m);
    TreeMap *(*Create)(size_t ElementSize);
    size_t (*GetElementSize)(TreeMap *d);
    TreeMap *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    DestructorFunction (*SetDestructor)(TreeMap *v,DestructorFunction fn);
    TreeMap *(*InitializeWith)(size_t ElementSize, size_t n, void *data);
    const ContainerAllocator *(*GetAllocator)(const TreeMap *t);
    
} TreeMapInterface;

extern TreeMapInterface iTreeMap;
/* -------------------------------------------------------------------------------------
 *                                                                                     *
 *                            Bit strings                                              *
 *                                                                                     *
 * ----------------------------------------------------------------------------------  */
typedef struct _BitString BitString;
#define BIT_TYPE unsigned char
#define BITSTRING_READONLY    1

/* Definition of the functions associated with this type. */
typedef struct tagBitString {
    size_t (*Size)(BitString *BitStr); /* Returns the number of elements stored */
    unsigned (*GetFlags)(BitString *BitStr); /* gets the flags */
    unsigned (*SetFlags)(BitString *BitStr,unsigned flags);/* Sets flags */
    int (*Clear)(BitString *BitStr);
    int (*Contains)(BitString *BitStr,BitString *str,void *ExtraArgs);
    int (*Erase)(BitString *BitStr,int bit);
    int         (*Finalize)(BitString *BitStr);
    int        (*Apply)(BitString *BitStr,int (*Applyfn)(int ,void * arg),void *arg);
    int       (*Equal)(BitString *bsl,BitString *bsr);
    BitString *(*Copy)(BitString *);
    ErrorFunction *(*SetErrorFunction)(BitString *,ErrorFunction fn);
    size_t     (*Sizeof)(BitString *b);
    Iterator  *(*NewIterator)(BitString *);
    int (*InitIterator)(BitString *,void *);
    int (*deleteIterator)(Iterator *);
    int        (*Save)(const BitString *bitstr,FILE *stream, SaveFunction saveFn,void *arg);
    BitString *(*Load)(FILE *stream, ReadFunction saveFn,void *arg);
    size_t (*GetElementSize)(BitString *b);

    /* Sequential container specific functions */
    
    int (*Add)(BitString *BitStr,int);
    int (*GetElement)(BitString *BitStr,size_t idx);
    int  (*PushBack)(BitString *BitStr,int val);
    int       (*PopBack)(BitString *BitStr);
    /* Inserts a string at the given position */
    size_t (*InsertAt)(BitString *BitStr,size_t idx,int bit);
    int (*EraseAt)(BitString *BitStr,size_t idx);
    int        (*ReplaceAt)(BitString *BitStr,size_t idx,int newval);
    int (*IndexOf)(BitString *BitStr,int SearchedBit,void *ExtraArgs,size_t *result);

    /* Bit string specific functions */
    
    /* Inserts a bit at the position zero. */
    size_t     (*Insert)(BitString *BitStr,int bit);
    int        (*SetElement)(BitString *bs,size_t position,int b);
    size_t     (*GetCapacity)(BitString *BitStr);
    int        (*SetCapacity)(BitString *BitStr,size_t newCapacity);
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
    BitString *(*InitializeWith)(size_t size,void *data);
    int        (*BitLeftShift)(BitString *bs,size_t shift);
    int        (*BitRightShift)(BitString *bs,size_t shift);
    size_t     (*Print)(BitString *b,size_t bufsiz,unsigned char *out);
    int        (*Append)(BitString *left,BitString *right);
    int        (*Memset)(BitString *,size_t start,size_t stop,int newval);
    BitString *(*CreateWithAllocator)(size_t startsiz,const ContainerAllocator *mm);
    BitString *(*Create)(size_t bitlen);
    BitString *(*Init)(BitString *BitStr,size_t bitlen);
    unsigned char *(*GetData)(BitString *BitStr);
    int        (*CopyBits)(BitString *bitstr,void *buf);
    int (*AddRange)(BitString *b, size_t bitSize, void *data);
    const ContainerAllocator *(*GetAllocator)(const BitString *b);
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
typedef void (*ObserverFunction)(const void *ObservedObject,unsigned operation,const void *ExtraInfo[]);

typedef struct tagObserverInterface {
    int (*Subscribe)(void *ObservedObject, ObserverFunction callback, unsigned Operations);
    int (*Notify)(const void *ObservedObject,unsigned operation,const void *ExtraInfo1,const void *ExtraInfo2);
    size_t (*Unsubscribe)(void *ObservedObject,ObserverFunction callback);
} ObserverInterface;
extern ObserverInterface iObserver;
#endif
