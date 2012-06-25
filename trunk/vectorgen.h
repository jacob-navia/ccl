#ifndef DATA_TYPE
#error "The symbol DATA_TYPE MUST be defined"
#else
#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif
#ifndef INT_MAX
#define INT_MAX (((unsigned)-1) >> 1)
#endif
#undef VECTOR_TYPE
#undef VECTOR_TYPE_
#undef INTERFACE
#undef ITERATOR
#undef ITERFACE_NAME
#undef VECTOR_ELEMENT
#undef INTERFACE_STRUCT_INTERNAL_NAME

#define CONCAT(x,y) x ## y
#define CONCAT3_(a,b,c) a##b##c
#define CONCAT3(a,b,c) CONCAT3_(a,b,c)
#define EVAL(t) t
#define INTERFACE(t) CONCAT(t,VectorInterface)
#define ITERATOR(t) CONCAT(t,VectorIterator)
#define ERROR_RETURN 0

#define VECTOR_TYPE_(t) CONCAT(t,Vector)
#define VECTOR_TYPE VECTOR_TYPE_(DATA_TYPE)
#define INTERFACE_NAME(a) CONCAT3(i,EVAL(a),Vector)
#define VECTOR_STRUCT_INTERNAL_NAME(a) CONCAT3(__,EVAL(a),Vector)
#define INTERFACE_STRUCT_INTERNAL_NAME(a) CONCAT3(__,EVAL(a),VectorInterface)

typedef struct VECTOR_STRUCT_INTERNAL_NAME(DATA_TYPE) VECTOR_TYPE;
typedef struct INTERFACE_STRUCT_INTERNAL_NAME(DATA_TYPE) INTERFACE(DATA_TYPE);
struct VECTOR_STRUCT_INTERNAL_NAME(DATA_TYPE) {
    INTERFACE(DATA_TYPE) *VTable;      /* Methods table */
    size_t count;               /* in elements units */
    unsigned Flags;
    unsigned timestamp;         /* Changed at each modification */
    size_t ElementSize;         /* Size (in bytes) of each element */
    DATA_TYPE *contents;        /* The data */
    CompareFunction CompareFn;  /* Element comparison function */
    ErrorFunction RaiseError;   /* Error function */
    ContainerHeap *Heap;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};

struct ITERATOR(DATA_TYPE) {
    Iterator it;
    VECTOR_TYPE *L;
    size_t index;
    unsigned  timestamp;
    unsigned long Flags;
    DATA_TYPE *Current;
    DATA_TYPE ElementBuffer;
    int   (*VectorReplace)(struct _Iterator *,void *data,int direction);
};
extern INTERFACE(DATA_TYPE) INTERFACE_NAME(DATA_TYPE);

struct INTERFACE_STRUCT_INTERNAL_NAME(DATA_TYPE) {
    size_t (*Size)(const VECTOR_TYPE *AL);
    unsigned (*GetFlags)(const VECTOR_TYPE *AL);
    unsigned (*SetFlags)(VECTOR_TYPE *AL,unsigned flags);
    int (*Clear)(VECTOR_TYPE *AL);
    /*Case sensitive search of a character string in the data */
    int (*Contains)(const VECTOR_TYPE *AL,const DATA_TYPE element,void *ExtraArgs);
    int (*Erase)(VECTOR_TYPE *AL,const DATA_TYPE elem);
    int (*EraseAll)(VECTOR_TYPE *AL,const DATA_TYPE elem);
    int (*Finalize)(VECTOR_TYPE *AL);
    int (*Apply)(VECTOR_TYPE *AL,int (*Applyfn)(DATA_TYPE *element,void * arg),void *arg);
    int (*Equal)(const VECTOR_TYPE *first,const VECTOR_TYPE *second);
    VECTOR_TYPE *(*Copy)(const VECTOR_TYPE *AL);
    ErrorFunction (*SetErrorFunction)(VECTOR_TYPE *AL,ErrorFunction);
    size_t (*Sizeof)(const VECTOR_TYPE *AL);
    Iterator *(*NewIterator)(VECTOR_TYPE *AL);
    int (*InitIterator)(VECTOR_TYPE *V,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const VECTOR_TYPE *);
    int (*Save)(const VECTOR_TYPE *AL,FILE *stream, SaveFunction saveFn,void *arg);
    VECTOR_TYPE *(*Load)(FILE *stream, ReadFunction readFn,void *arg);
    size_t (*GetElementSize)(const VECTOR_TYPE *AL);

    /* Sequential container specific fields */

    int (*Add)(VECTOR_TYPE *AL,const DATA_TYPE newval);
    DATA_TYPE *(*GetElement)(const VECTOR_TYPE *AL,size_t idx);
    int (*PushBack)(VECTOR_TYPE *AL,const DATA_TYPE str);
    int (*PopBack)(VECTOR_TYPE *AL,DATA_TYPE result);
    int (*InsertAt)(VECTOR_TYPE *AL,size_t idx,DATA_TYPE newval);
    int (*EraseAt)(VECTOR_TYPE *AL,size_t idx);
    int (*ReplaceAt)(VECTOR_TYPE *AL,size_t idx,DATA_TYPE newval);
    int (*IndexOf)(const VECTOR_TYPE *AL,const DATA_TYPE data,void *ExtraArgs,size_t *result);

    /* VECTOR_TYPE container specific fields */

    int (*Insert)(VECTOR_TYPE *AL,DATA_TYPE elem);
    int (*InsertIn)(VECTOR_TYPE *AL, size_t idx,VECTOR_TYPE *newData);
    VECTOR_TYPE *(*IndexIn)(VECTOR_TYPE *SC,const Vector *AL);
    size_t (*GetCapacity)(const VECTOR_TYPE *AL);
    int (*SetCapacity)(VECTOR_TYPE *AL,size_t newCapacity);

    CompareFunction (*SetCompareFunction)(VECTOR_TYPE *l,CompareFunction fn);
    int (*Sort)(VECTOR_TYPE *AL);
    VECTOR_TYPE *(*Create)(size_t startsize);
    VECTOR_TYPE *(*CreateWithAllocator)(size_t startsiz,const ContainerAllocator *mm);
    VECTOR_TYPE *(*Init)(VECTOR_TYPE *r,size_t startsize);
    int (*AddRange)(VECTOR_TYPE *AL,size_t n,const DATA_TYPE *newvalues);
    VECTOR_TYPE *(*GetRange)(const VECTOR_TYPE *AL, size_t start, size_t end);
    int (*CopyElement)(const VECTOR_TYPE *AL,size_t idx,DATA_TYPE *outbuf);
    void **(*CopyTo)(const VECTOR_TYPE *AL);
    int (*Reverse)(VECTOR_TYPE *AL);
    int (*Append)(VECTOR_TYPE *AL1, VECTOR_TYPE *AL2);
    int (*Mismatch)(VECTOR_TYPE *a1,VECTOR_TYPE *a2,size_t *mismatch);
    const ContainerAllocator *(*GetAllocator)(const VECTOR_TYPE *AL);
    DestructorFunction (*SetDestructor)(VECTOR_TYPE *v,DestructorFunction fn);
    int (*SearchWithKey)(VECTOR_TYPE *vec,size_t startByte,size_t sizeKey,size_t startIndex,DATA_TYPE item,size_t *result);
    int (*Select)(VECTOR_TYPE *src,const Mask *m);
    VECTOR_TYPE *(*SelectCopy)(VECTOR_TYPE *src,Mask *m);
    int (*Resize)(VECTOR_TYPE *AL,size_t newSize);
    VECTOR_TYPE *(*InitializeWith)(size_t n,const DATA_TYPE *Data);
    DATA_TYPE *(*GetData)(const VECTOR_TYPE *AL);
    DATA_TYPE *(*Back)(const VECTOR_TYPE *AL);
    DATA_TYPE *(*Front)(const VECTOR_TYPE *AL);
    int (*RemoveRange)(VECTOR_TYPE *SC,size_t start,size_t end);
    int (*RotateLeft)(VECTOR_TYPE *V,size_t n);
    int (*RotateRight)(VECTOR_TYPE *V,size_t n);
    Mask *(*CompareEqual)(const VECTOR_TYPE *left,const VECTOR_TYPE *right,Mask *m);
    Mask *(*CompareEqualScalar)(const VECTOR_TYPE *left, const DATA_TYPE right,Mask *m);
    int (*Reserve)(VECTOR_TYPE *src,size_t newCapacity);
};
#endif
