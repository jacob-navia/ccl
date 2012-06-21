#ifndef __dlistgen_h__
#define __dlistgen_h__
#ifndef DATA_TYPE
#error "The symbol DATA_TYPE MUST be defined"
#else
#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif
#ifndef INT_MAX
#define INT_MAX (((unsigned)-1) >> 1)
#endif
#undef LIST_TYPE
#undef LIST_TYPE_
#undef INTERFACE
#undef ITERATOR
#undef ITERFACE_NAME
#undef LIST_ELEMENT
#undef LIST_ELEMENT_
#undef LIST_STRUCT_INTERNAL_NAME
#undef INTERFACE_STRUCT_INTERNAL_NAME

#define CONCAT(x,y) x ## y
#define CONCAT3_(a,b,c) a##b##c
#define CONCAT3(a,b,c) CONCAT3_(a,b,c)
#define EVAL(t) t
#define LIST_ELEMENT_(t) CONCAT(t,DlistElement)
#define LIST_ELEMENT LIST_ELEMENT_(DATA_TYPE)
#define INTERFACE(t) CONCAT(t,DlistInterface)
#define ITERATOR(t) CONCAT(t,DlistIterator)
#define ERROR_RETURN 0

#define LIST_TYPE_(t) CONCAT(t,Dlist)
#define LIST_TYPE LIST_TYPE_(DATA_TYPE)
#define INTERFACE_NAME(a) CONCAT3(i,EVAL(a),Dlist)
#define LIST_STRUCT_INTERNAL_NAME(a) CONCAT3(__,EVAL(a),Dlist)
#define INTERFACE_STRUCT_INTERNAL_NAME(a) CONCAT3(__,EVAL(a),DlistInterface)

typedef struct LIST_ELEMENT {
    struct LIST_ELEMENT *Next;
    struct LIST_ELEMENT *Previous;
    DATA_TYPE Data;
} LIST_ELEMENT;

typedef struct LIST_STRUCT_INTERNAL_NAME(DATA_TYPE) LIST_TYPE;
typedef struct INTERFACE_STRUCT_INTERNAL_NAME(DATA_TYPE) INTERFACE(DATA_TYPE);
struct LIST_STRUCT_INTERNAL_NAME(DATA_TYPE) {
    INTERFACE(DATA_TYPE) *VTable;      /* Methods table */
    size_t count;               /* in elements units */
    unsigned Flags;
    unsigned timestamp;         /* Changed at each modification */
    size_t ElementSize;         /* Size (in bytes) of each element */
    LIST_ELEMENT *Last;         /* The last item */
    LIST_ELEMENT *First;        /* The contents of the list start here */
    CompareFunction Compare;    /* Element comparison function */
    ErrorFunction RaiseError;   /* Error function */
    ContainerHeap *Heap;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};

struct ITERATOR(DATA_TYPE) {
    Iterator it;
    LIST_TYPE *L;
    size_t index;
    LIST_ELEMENT *Current;
    LIST_ELEMENT *Previous;
    unsigned  timestamp;
    DATA_TYPE ElementBuffer;
    int   (*DlistReplace)(struct _Iterator *,void *data,int direction);
};
extern INTERFACE(DATA_TYPE) INTERFACE_NAME(DATA_TYPE);

struct INTERFACE_STRUCT_INTERNAL_NAME(DATA_TYPE) {
    size_t (*Size)(const LIST_TYPE *dl);
    unsigned (*GetFlags)(const LIST_TYPE *AL);
    unsigned (*SetFlags)(LIST_TYPE *AL,unsigned flags);
    int (*Clear)(LIST_TYPE *dl);
    int (*Contains)(const LIST_TYPE *dl,const DATA_TYPE element);
    int (*Erase)(LIST_TYPE *AL,const DATA_TYPE);
    int (*EraseAll)(LIST_TYPE *AL,const DATA_TYPE);
    int (*Finalize)(LIST_TYPE *AL);
    int (*Apply)(LIST_TYPE *L,int(Applyfn)(DATA_TYPE *elem,void *extraArg),void *extraArg);
    int (*Equal)(const LIST_TYPE *l1,const LIST_TYPE *l2);
    LIST_TYPE *(*Copy)(const LIST_TYPE *dl);
    ErrorFunction (*SetErrorFunction)(LIST_TYPE *L,ErrorFunction);
    size_t (*Sizeof)(const LIST_TYPE *dl);
    Iterator *(*NewIterator)(LIST_TYPE *);
    int (*InitIterator)(LIST_TYPE *,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const LIST_TYPE *);
    int (*Save)(const LIST_TYPE *L,FILE *stream, SaveFunction saveFn,void *arg);
    LIST_TYPE *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    size_t (*GetElementSize)(const LIST_TYPE *dl);

    /* -----------------------------------------Sequential container part */
    int (*Add)(LIST_TYPE *dl,const DATA_TYPE newval);
    DATA_TYPE *(*GetElement)(const LIST_TYPE *AL,size_t idx);
    int (*PushFront)(LIST_TYPE *AL,const DATA_TYPE str);
    int (*PopFront)(LIST_TYPE *AL,DATA_TYPE *result);
    int (*InsertAt)(LIST_TYPE *AL,size_t idx,const DATA_TYPE newval);
    int (*EraseAt)(LIST_TYPE *AL,size_t idx);
    int (*ReplaceAt)(LIST_TYPE *AL,size_t idx,const DATA_TYPE newval);
    int (*IndexOf)(const LIST_TYPE *AL,const DATA_TYPE SearchedElement,void *args,size_t *result);

    /* ------------------------------------LIST_TYPE container specific part */
    int (*PushBack)(LIST_TYPE *AL,const DATA_TYPE str);
    int (*PopBack)(LIST_TYPE *AL,DATA_TYPE *result);
    LIST_TYPE *(*Splice)(LIST_TYPE *list,void *pos,LIST_TYPE *toInsert,int direction);
    int (*Sort)(LIST_TYPE *l);
    int (*Reverse)(LIST_TYPE *l);
    LIST_TYPE *(*GetRange)(const LIST_TYPE *l,size_t start,size_t end);
    int (*Append)(LIST_TYPE *l1,LIST_TYPE *l2);
    CompareFunction (*SetCompareFunction)(LIST_TYPE *l,CompareFunction fn);
    int (*UseHeap)(LIST_TYPE *L,const ContainerAllocator *m);
    int (*AddRange)(LIST_TYPE *l,size_t n,const DATA_TYPE *data);
    LIST_TYPE *(*Create)(size_t elementsize);
    LIST_TYPE *(*CreateWithAllocator)(size_t,const ContainerAllocator *);
    LIST_TYPE *(*Init)(LIST_TYPE *dlist,size_t elementsize);
    LIST_TYPE *(*InitWithAllocator)(LIST_TYPE *L,size_t element_size, const ContainerAllocator *mm);
    int (*CopyElement)(const LIST_TYPE *l,size_t idx,DATA_TYPE *outbuf);
    int (*InsertIn)(LIST_TYPE *l, size_t idx,LIST_TYPE *newData);
    DestructorFunction (*SetDestructor)(LIST_TYPE *v,DestructorFunction fn);
    LIST_TYPE *(*InitializeWith)(size_t elementSize, size_t n,const DATA_TYPE *data);
    const ContainerAllocator *(*GetAllocator)(const LIST_TYPE *l);
    DATA_TYPE *(*Back)(const LIST_TYPE *l);
    DATA_TYPE *(*Front)(const LIST_TYPE *l);
    int (*RemoveRange)(LIST_TYPE *l,size_t start, size_t end);
    int (*RotateLeft)(LIST_TYPE *l, size_t n);
    int (*RotateRight)(LIST_TYPE *l,size_t n);
    int (*Select)(LIST_TYPE *src,const Mask *m);
    LIST_TYPE *(*SelectCopy)(const LIST_TYPE *src,const Mask *m);
    LIST_ELEMENT *(*FirstElement)(LIST_TYPE *l);
    LIST_ELEMENT *(*LastElement)(LIST_TYPE *l);
    LIST_ELEMENT *(*NextElement)(LIST_ELEMENT *le);
    LIST_ELEMENT *(*PreviousElement)(LIST_ELEMENT *le);
    DATA_TYPE *(*ElementData)(LIST_ELEMENT *le);
    int (*SetElementData)(LIST_TYPE *l, LIST_ELEMENT *le,DATA_TYPE data);
    DATA_TYPE *(*Advance)(LIST_ELEMENT **pLIST_ELEMENT);
    LIST_ELEMENT *(*Skip)(LIST_ELEMENT *l,size_t n);
    void *(*MoveBack)(LIST_ELEMENT **pLIST_ELEMENT);
    LIST_TYPE *(*SplitAfter)(LIST_TYPE *l, LIST_ELEMENT *pt);
};
#endif
#endif
