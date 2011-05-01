/****************************************************************************
 *          ValArrays                                                     *
 ****************************************************************************/
typedef struct _ValArray ValArray;
typedef struct tagValArray {
    size_t (*Size)(const ValArray *AL);
    unsigned (*GetFlags)(const ValArray *AL);
    unsigned (*SetFlags)(ValArray *AL,unsigned flags);
    int (*Clear)(ValArray *AL);
    int (*Contains)(ValArray *AL,ElementType data);
    int (*Erase)(ValArray *AL,ElementType elem);
    int (*Finalize)(ValArray *AL);
    int (*Apply)(ValArray *AL,int (*Applyfn)(ElementType element,void * arg),void *arg);
    int (*Equal)(const ValArray *first, const ValArray *second);
    ValArray *(*Copy)(ValArray *AL);
    ErrorFunction (*SetErrorFunction)(ValArray *AL,ErrorFunction);
    size_t (*Sizeof)(ValArray *AL);
    Iterator *(*newIterator)(ValArray *AL);
    int (*deleteIterator)(Iterator *);
    int (*Save)(ValArray *AL,FILE *stream);
    ValArray *(*Load)(FILE *stream);
    size_t (*GetElementSize)(const ValArray *AL);

        /* Sequential container specific fields */

    int (*Add)(ValArray *AL,ElementType newval);
    ElementType (*GetElement)(const ValArray *AL,size_t idx);
    int (*PushBack)(ValArray *AL,ElementType data);
    int (*PopBack)(ValArray *AL,ElementType *result);
    int (*InsertAt)(ValArray *AL,size_t idx,ElementType newval);
    int (*EraseAt)(ValArray *AL,size_t idx);
    int (*ReplaceAt)(ValArray *AL,size_t idx,ElementType newval);
    /* NO extra args */
    int (*IndexOf)(ValArray *AL,ElementType data,size_t *result);

        /* ValArray container specific fields */

    int (*Insert)(ValArray *AL,ElementType);
    int (*InsertIn)(ValArray *AL, size_t idx, ValArray *newData);
    ValArray *(*IndexIn)(const ValArray *SC,const ValArraySize_t *AL);
    size_t (*GetCapacity)(const ValArray *AL);
    int (*SetCapacity)(ValArray *AL,size_t newCapacity);

    CompareFunction (*SetCompareFunction)(ValArray *l,CompareFunction fn);
    int (*Sort)(ValArray *AL);
    ValArray *(*Create)(size_t startsize);
    ValArray *(*CreateWithAllocator)(size_t startsize, ContainerMemoryManager *allocator);
    ValArray *(*Init)(ValArray *AL,size_t startsize);
    int (*AddRange)(ValArray *AL,size_t n, const ElementType *newvalues);
    ValArray *(*GetRange)(const ValArray *AL, size_t start, size_t end);
    int (*CopyElement)(ValArray *AL,size_t idx,ElementType *outbuf);
    ElementType *(*CopyTo)(ValArray *AL);
    int (*Reverse)(ValArray *AL);
    int (*Append)(ValArray *AL1, ValArray *AL2);
    int (*Mismatch)(ValArray *a1, ValArray *a2,size_t *mismatch);
    ContainerMemoryManager *(*GetAllocator)(ValArray *AL);
    DestructorFunction (*SetDestructor)(ValArray *cb,DestructorFunction fn);

    /* ValArray specific functions */
    ErrorFunction RaiseError;      /* Error function */
    int (*SumTo)(ValArray *left,const ValArray *right);
    int (*SubtractFrom)(ValArray *left, const ValArray *right);
    int (*MultiplyWith)(ValArray *left, const ValArray *right);
    int (*DivideBy)(ValArray *left, const ValArray *right);
    int (*SumScalarTo)(ValArray *left,ElementType right);
    int (*SubtractScalarFrom)(ValArray *left, ElementType right);
    int (*SubtractFromScalar)(ElementType left, ValArray *right);
    int (*MultiplyWithScalar)(ValArray *left, ElementType right);
    int (*DivideByScalar)(ValArray *left, ElementType right);
    int (*DivideScalarBy)(ElementType left,ValArray *right);
    Mask *(*CompareEqual)(const ValArray *left,const ValArray *right,Mask *bytearray);
    Mask *(*CompareEqualScalar)(const ValArray *left, const ElementType right, Mask *bytearray);
    char *(*Compare)(const ValArray *left, const ValArray *right,char *bytearray);
    char *(*CompareScalar)(const ValArray *left, const ElementType right,char *bytearray);

    ValArray *(*CreateSequence)(size_t n,ElementType start, ElementType increment);
    ValArray *(*InitializeWith)(size_t n, ElementType *data);
    int (*Memset)(ValArray *dst,ElementType fillValue,size_t length);
    int (*FillSequential)(ValArray *dst,size_t length,ElementType start, ElementType increment);
    int (*SetSlice)(ValArray *src,size_t start,size_t length,size_t increment);
    int (*ResetSlice)(ValArray *array);
    int (*GetSlice)(ValArray *array,size_t *start,size_t *length, size_t *increment);
    ElementType (*Max)(const ValArray *src);
    ElementType (*Min)(const ValArray *src);
    int (*RotateLeft)(ValArray *AL, size_t n);
    int (*RotateRight)(ValArray *AL,size_t n);
#ifdef __IS_UNSIGNED__
    int (*Or)(ValArray *left, const ValArray *right);
    int (*And)(ValArray *left, const ValArray *right);
    int (*Xor)(ValArray *left, const ValArray *right);
    int (*Not)(ValArray *left);
    int (*BitLeftShift)(ValArray *data,int shift);
    int (*BitRightShift)(ValArray *data, const int shift);
    int (*OrScalar)(ValArray *left, const ElementType right);
    int (*AndScalar)(ValArray *left, const ElementType right);
    int (*XorScalar)(ValArray *left, const ElementType right);
#else
    int (*Abs)(ValArray *src); /* Abs only defined for float/signed types */
#endif
#ifdef __IS_INTEGER__
    int (*Mod)(ValArray *left,const ValArray *right);
    int (*ModScalar)(ValArray *left,const ElementType right);
#else
    char *(*FCompare)(const ValArray *left, const ValArray *right,char *bytearray,ElementType tolerance);
    int (*Inverse)(ValArray *src);
#endif
    int (*ForEach)(ValArray *src,ElementType (*ApplyFn)(ElementType));
    ElementType (*Accumulate)(ValArray *src);
    ElementType (*Product)(ValArray *src);
    int (*Fprintf)(ValArray *src,FILE *out,const char *fmt);
    int (*Select)(ValArray *src,const Mask *m);
    ValArray *(*SelectCopy)(const ValArray *src,const Mask *m);
} ValArrayInterface;
