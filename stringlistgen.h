#ifndef __containers_h__
#include "containers.h"
#endif
#undef CONCAT
#undef CONCAT3
#undef EVAL
#undef iSTRINGLIST
#undef INTERFACE
#undef LIST_ELEMENT
#undef LIST_TYPE
#undef ITERATOR
#undef INTERFACE_STRUCT_INTERNAL_NAME
#undef LIST_STRUCT_INTERNAL_NAME

#define CONCAT(x,y) x ## y
#define CONCAT3_(a,b,c) a##b##c
#define CONCAT3(a,b,c) CONCAT3_(a,b,c)
#define EVAL(t) t
#define iSTRINGLIST(t) CONCAT3(i,t,Interface) 
#define INTERFACE(t) CONCAT(t,ListInterface)
#define LIST_ELEMENT(t) CONCAT(t,ListElement)
#define LIST_TYPE(t) CONCAT(t,List)
#define ITERATOR(t) CONCAT(t,ListIterator)
#define INTERFACE_STRUCT_INTERNAL_NAME(a) CONCAT3(__,EVAL(a),ListInterface)
#define LIST_STRUCT_INTERNAL_NAME(a) CONCAT3(__,EVAL(a),List)

#ifdef NO_C99
/* No flexible arrays */
#define MINIMUM_ARRAY_INDEX     1
#else
/* Use C99 features */
#define MINIMUM_ARRAY_INDEX
#endif

/*----------------------------------------------------------------------------*/
/* Definition of the stringlist and stringlist element type                   */
/*----------------------------------------------------------------------------*/
typedef struct LIST_ELEMENT(DATA_TYPE) {
    struct LIST_ELEMENT(DATA_TYPE) *Next;
    CHARTYPE Data[MINIMUM_ARRAY_INDEX];
} LIST_ELEMENT(DATA_TYPE);

typedef struct INTERFACE_STRUCT_INTERNAL_NAME(DATA_TYPE) INTERFACE(DATA_TYPE);
typedef struct LIST_STRUCT_INTERNAL_NAME(DATA_TYPE) {
    INTERFACE(DATA_TYPE) *VTable;      /* Methods table */
    size_t count;               /* in elements units */
    unsigned Flags;
    unsigned timestamp;         /* Changed at each modification */
    size_t ElementSize;         /* Size (in bytes) of each element */
    LIST_ELEMENT(DATA_TYPE) *Last;         /* The last item */
    LIST_ELEMENT(DATA_TYPE) *First;        /* The contents of the list start here */
    CompareFunction Compare;    /* Element comparison function */
    ErrorFunction RaiseError;   /* Error function */
    ContainerHeap *Heap;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
} LIST_TYPE(DATA_TYPE);

struct ITERATOR(DATA_TYPE) {
    Iterator it;
    LIST_TYPE(DATA_TYPE) *L;
    size_t index;
    LIST_ELEMENT(DATA_TYPE) *Current;
    LIST_ELEMENT(DATA_TYPE) *Previous;
    unsigned timestamp;
    CHARTYPE *ElementBuffer;
};

struct INTERFACE_STRUCT_INTERNAL_NAME(DATA_TYPE) {
    size_t (*Size)(const LIST_TYPE(DATA_TYPE) *L);        /* Returns the number of elements stored */
    unsigned (*GetFlags)(const LIST_TYPE(DATA_TYPE) *L);  /* Gets the flags */
    unsigned (*SetFlags)(LIST_TYPE(DATA_TYPE) *L,unsigned flags);       /* Sets the flags */
    int (*Clear)(LIST_TYPE(DATA_TYPE) *L);                         /* Clears all elements */
    int (*Contains)(const LIST_TYPE(DATA_TYPE) *L,const CHARTYPE *element);       /* Searches an element */
    int (*Erase)(LIST_TYPE(DATA_TYPE) *L,CHARTYPE *);       /* erases the given data if found */
    /* Frees the memory used by the collection and destroys the list */
    int (*Finalize)(LIST_TYPE(DATA_TYPE) *L);
    /* Call a callback function with each element of the list and an extra argument */
    int (*Apply)(LIST_TYPE(DATA_TYPE) *L,int(Applyfn)(CHARTYPE *,void *),void *arg);
    int (*Equal)(LIST_TYPE(DATA_TYPE) *l1,LIST_TYPE(DATA_TYPE) *l2);  /* Compares two lists (Shallow comparison) */
    LIST_TYPE(DATA_TYPE) *(*Copy)(const LIST_TYPE(DATA_TYPE) *L);           /* Copies all items into a new list */
    ErrorFunction (*SetErrorFunction)(LIST_TYPE(DATA_TYPE) *L,ErrorFunction); /* Set/unset the error function */
    size_t (*Sizeof)(LIST_TYPE(DATA_TYPE) *l);
    Iterator *(*NewIterator)(LIST_TYPE(DATA_TYPE) *L);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(LIST_TYPE(DATA_TYPE) *l);
    int (*Save)(LIST_TYPE(DATA_TYPE) *L,FILE *stream, SaveFunction saveFn,void *arg);
    LIST_TYPE(DATA_TYPE) *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    size_t (*GetElementSize)(LIST_TYPE(DATA_TYPE) *l);
 /* -------------------------------------------This is the Sequential container part */
    int (*Add)(LIST_TYPE(DATA_TYPE) *L,CHARTYPE *newval);              /* Adds element at end */
    /* Returns the data at the given position */
    CHARTYPE *(*GetElement)(LIST_TYPE(DATA_TYPE) *L,size_t idx);
    /* Pushes an element, using the collection as a stack */
    int (*PushFront)(LIST_TYPE(DATA_TYPE) *L,CHARTYPE *str);
    /* Pops the first element the list */
    int (*PopFront)(LIST_TYPE(DATA_TYPE) *L,CHARTYPE *result);
    /* Inserts a value at the given position */
    int (*InsertAt)(LIST_TYPE(DATA_TYPE) *L,size_t idx,CHARTYPE *newval);
    int (*EraseAt)(LIST_TYPE(DATA_TYPE) *L,size_t idx);
    /* Replaces the element at the given position with a new one */
    int (*ReplaceAt)(LIST_TYPE(DATA_TYPE) *L,size_t idx,CHARTYPE *newval);
    /*Returns the index of the given data or -1 if not found */
    int (*IndexOf)(LIST_TYPE(DATA_TYPE) *L,CHARTYPE *SearchedElement,void *ExtraArgs,size_t *result);
/* -------------------------------------------This is the list container part */
    int (*InsertIn)(LIST_TYPE(DATA_TYPE) *l, size_t idx,LIST_TYPE(DATA_TYPE) *newData);
    int  (*CopyElement)(LIST_TYPE(DATA_TYPE) *list,size_t idx,CHARTYPE *OutBuffer);
    /*erases the string at the indicated position */
    int (*EraseRange)(LIST_TYPE(DATA_TYPE) *L,size_t start,size_t end);
    /* Sorts the list in place */
    int (*Sort)(LIST_TYPE(DATA_TYPE) *l);
    /* Reverses the list */
    int (*Reverse)(LIST_TYPE(DATA_TYPE) *l);
    /* Gets an element range from the list */
    LIST_TYPE(DATA_TYPE) *(*GetRange)(LIST_TYPE(DATA_TYPE) *l,size_t start,size_t end);
    /* Add a list at the end of another */
    int (*Append)(LIST_TYPE(DATA_TYPE) *l1,LIST_TYPE(DATA_TYPE) *l2);
    /* Set the comparison function */
    CompareFunction (*SetCompareFunction)(LIST_TYPE(DATA_TYPE) *l,CompareFunction fn);
    CompareFunction Compare;
    int (*UseHeap)(LIST_TYPE(DATA_TYPE) *L, ContainerAllocator *m);
    int (*AddRange)(LIST_TYPE(DATA_TYPE) *L, size_t n,CHARTYPE **data);
    LIST_TYPE(DATA_TYPE) *(*Create)(void);
    LIST_TYPE(DATA_TYPE) *(*CreateWithAllocator)(const ContainerAllocator *allocator);
    LIST_TYPE(DATA_TYPE) *(*Init)(LIST_TYPE(DATA_TYPE) *);
    LIST_TYPE(DATA_TYPE) *(*InitWithAllocator)(LIST_TYPE(DATA_TYPE) *a,ContainerAllocator *allocator);
    LIST_TYPE(DATA_TYPE) *(*SetAllocator)(LIST_TYPE(DATA_TYPE) *l, ContainerAllocator  *allocator);
    int (*InitIterator)(LIST_TYPE(DATA_TYPE) *list,void *storage);
    const ContainerAllocator *(*GetAllocator)(LIST_TYPE(DATA_TYPE) *list);
    DestructorFunction (*SetDestructor)(LIST_TYPE(DATA_TYPE) *v,DestructorFunction fn);
    LIST_TYPE(DATA_TYPE) *(*InitializeWith)(size_t n,CHARTYPE **data);
    CHARTYPE *(*Back)(const LIST_TYPE(DATA_TYPE) *l);
    CHARTYPE *(*Front)(const LIST_TYPE(DATA_TYPE) *l);
    int (*Select)(LIST_TYPE(DATA_TYPE) *src,const Mask *m);
    LIST_TYPE(DATA_TYPE) *(*SelectCopy)(const LIST_TYPE(DATA_TYPE) *src, const Mask *m);
    LIST_ELEMENT(DATA_TYPE) *(*FirstElement)(LIST_TYPE(DATA_TYPE) *l);
    LIST_ELEMENT(DATA_TYPE) *(*LastElement)(LIST_TYPE(DATA_TYPE) *l);
    LIST_ELEMENT(DATA_TYPE) *(*NextElement)(LIST_ELEMENT(DATA_TYPE) *le);
    void *(*ElementData)(LIST_ELEMENT(DATA_TYPE) *le);
    int (*SetElementData)(LIST_TYPE(DATA_TYPE) *l, LIST_ELEMENT(DATA_TYPE) **pple,const CHARTYPE *data);
    void *(*Advance)(LIST_ELEMENT(DATA_TYPE) **p);
    LIST_ELEMENT(DATA_TYPE) *(*Skip)(LIST_ELEMENT(DATA_TYPE) *l,size_t n);
    LIST_TYPE(DATA_TYPE) *(*SplitAfter)(LIST_TYPE(DATA_TYPE) *l, LIST_ELEMENT(DATA_TYPE) *pt);
};

extern INTERFACE(DATA_TYPE) iSTRINGLIST(DATA_TYPE);

