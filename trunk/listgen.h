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

#define CONCAT(x,y) x ## y
#define CONCAT3_(a,b,c) a##b##c
#define CONCAT3(a,b,c) CONCAT3_(a,b,c)
#define EVAL(t) t
#define INTERFACE(t) CONCAT(t,ListInterface)
#define ITERATOR(t) CONCAT(t,ListIterator)
#define ERROR_RETURN 0

#define LIST_TYPE_(t) CONCAT(t,List)
#define LIST_TYPE LIST_TYPE_(DATA_TYPE)
#define INTERFACE_NAME(a) CONCAT3(i,EVAL(a),List)
#define LIST_STRUCT_INTERNAL_NAME(a) CONCAT3(__,EVAL(a),List)
#define INTERFACE_STRUCT_INTERNAL_NAME(a) CONCAT3(__,EVAL(a),ListInterface)

typedef struct LIST_ELEMENT {
    struct LIST_ELEMENT *Next;
#ifdef SPARC32
    void *alignment;
#endif
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
    int   (*ListReplace)(struct _Iterator *,void *data,int direction);
};
extern INTERFACE(DATA_TYPE) INTERFACE_NAME(DATA_TYPE);

struct INTERFACE_STRUCT_INTERNAL_NAME(DATA_TYPE) {
    size_t (*Size)(const LIST_TYPE *L);
    unsigned (*GetFlags)(const LIST_TYPE *L);
    unsigned (*SetFlags)(LIST_TYPE *L,unsigned flags);
    int (*Clear)(LIST_TYPE *L);
    int (*Contains)(const LIST_TYPE *L,const DATA_TYPE element);
    int (*Erase)(LIST_TYPE *L,const DATA_TYPE);
    int (*EraseAll)(LIST_TYPE *l,const DATA_TYPE);
    int (*Finalize)(LIST_TYPE *L);
    int (*Apply)(LIST_TYPE *L,int(Applyfn)(DATA_TYPE *,void *),void *arg);
    int (*Equal)(const LIST_TYPE *l1,const LIST_TYPE *l2);
    LIST_TYPE *(*Copy)(const LIST_TYPE *L);
    ErrorFunction (*SetErrorFunction)(LIST_TYPE *L,ErrorFunction);
    size_t (*Sizeof)(const LIST_TYPE *l);
    Iterator *(*NewIterator)(LIST_TYPE *L);
    int (*InitIterator)(LIST_TYPE *L,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const LIST_TYPE *);
    int (*Save)(const LIST_TYPE *L,FILE *stream, SaveFunction saveFn,void *arg);
    LIST_TYPE *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    size_t (*GetElementSize)(const LIST_TYPE *l);
 /* -------------------------------------------This is the Sequential container part */
    int (*Add)(LIST_TYPE *L,const DATA_TYPE newval);
    DATA_TYPE *(*GetElement)(const LIST_TYPE *L,size_t idx);
    int (*PushFront)(LIST_TYPE *L,const DATA_TYPE str);
    int (*PopFront)(LIST_TYPE *L,DATA_TYPE *result);
    int (*InsertAt)(LIST_TYPE *L,size_t idx,const DATA_TYPE newval);
    int (*EraseAt)(LIST_TYPE *L,size_t idx);
    int (*ReplaceAt)(LIST_TYPE *L,size_t idx,const DATA_TYPE newval);
    int (*IndexOf)(const LIST_TYPE *L,const DATA_TYPE SearchedElement,void *ExtraArgs,size_t *result);
/* -------------------------------------------This is the list container part */
    int (*InsertIn)(LIST_TYPE *l, size_t idx,LIST_TYPE *newData);
    int  (*CopyElement)(const LIST_TYPE *list,size_t idx,DATA_TYPE *OutBuffer);
    int (*EraseRange)(LIST_TYPE *L,size_t start,size_t end);
    int (*Sort)(LIST_TYPE *l);
    int (*Reverse)(LIST_TYPE *l);
    LIST_TYPE *(*GetRange)(const LIST_TYPE *l,size_t start,size_t end);
    int (*Append)(LIST_TYPE *l1,LIST_TYPE *l2);
    CompareFunction (*SetCompareFunction)(LIST_TYPE *l,CompareFunction fn);
    CompareFunction Compare;
    int (*UseHeap)(LIST_TYPE *L, const ContainerAllocator *m);
    int (*AddRange)(LIST_TYPE *L, size_t n,const DATA_TYPE *data);
    LIST_TYPE *(*Create)(void);
    LIST_TYPE *(*CreateWithAllocator)(const ContainerAllocator *mm);
    LIST_TYPE *(*Init)(LIST_TYPE *aList);
    LIST_TYPE *(*InitWithAllocator)(LIST_TYPE *aList,const ContainerAllocator *mm);
    const ContainerAllocator *(*GetAllocator)(const LIST_TYPE *list);
    DestructorFunction (*SetDestructor)(LIST_TYPE *v,DestructorFunction fn);
    LIST_TYPE *(*InitializeWith)(size_t n,const void *data);
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
    DATA_TYPE *(*ElementData)(LIST_ELEMENT *le);
    int (*SetElementData)(LIST_TYPE *l, LIST_ELEMENT *le,DATA_TYPE data);
    DATA_TYPE *(*Advance)(LIST_ELEMENT **);
    LIST_ELEMENT *(*Skip)(LIST_ELEMENT *l,size_t n);
    LIST_TYPE *(*SplitAfter)(LIST_TYPE *l, LIST_ELEMENT *pt);
};
#endif
