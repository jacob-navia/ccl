/****************************************************************************
 *          ValArrays                                                     *
 ****************************************************************************/
typedef struct _ValArray ValArray;
typedef struct {
    size_t (*Size)(const ValArray *AL);
    unsigned (*GetFlags)(const ValArray *AL);
    unsigned (*SetFlags)(ValArray *AL,unsigned flags);
    int (*Clear)(ValArray *AL);
    int (*Contains)(const ValArray *AL,ElementType data);
    int (*Erase)(ValArray *AL,ElementType elem);
    int (*Finalize)(ValArray *AL);
    int (*Apply)(ValArray *AL,int (*Applyfn)(ElementType element,void * arg),void *arg);
    int (*Equal)(const ValArray *first, const ValArray *second);
    ValArray *(*Copy)(const ValArray *AL);
    ErrorFunction (*SetErrorFunction)(ValArray *AL,ErrorFunction);
    size_t (*Sizeof)(const ValArray *AL);
    Iterator *(*NewIterator)(ValArray *AL);
    int (*InitIterator)(ValArray *AL,void *buf);
    int (*deleteIterator)(Iterator *);
    size_t (*SizeofIterator)(const ValArray *);
    int (*Save)(const ValArray *AL,FILE *stream);
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
    ValArray *(*CreateWithAllocator)(size_t startsize, ContainerAllocator *allocator);
    ValArray *(*Init)(ValArray *AL,size_t startsize);
    int (*AddRange)(ValArray *AL,size_t n, const ElementType *newvalues);
    ValArray *(*GetRange)(const ValArray *AL, size_t start, size_t end);
    int (*CopyElement)(const ValArray *AL,size_t idx,ElementType *outbuf);
    ElementType *(*CopyTo)(ValArray *AL);
    int (*Reverse)(ValArray *AL);
    int (*Append)(ValArray *AL1, ValArray *AL2);
    int (*Mismatch)(const ValArray *a1,const ValArray *a2,size_t *mismatch);
    ContainerAllocator *(*GetAllocator)(const ValArray *AL);
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
    int (*DivideScalarBy)(ValArray *left,ElementType right);
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
    Mask *(*FCompare)(const ValArray *left, const ValArray *right, Mask *bytearray,ElementType tolerance);
    int (*Inverse)(ValArray *src);
#endif
    int (*ForEach)(ValArray *src,ElementType (*ApplyFn)(ElementType));
    ElementType (*Accumulate)(const ValArray *src);
    ElementType (*Product)(const ValArray *src);
    int (*Fprintf)(const ValArray *src,FILE *out,const char *fmt);
    int (*Select)(ValArray *src,const Mask *m);
    ValArray *(*SelectCopy)(const ValArray *src,const Mask *m);
    ElementType *(*GetData)(const ValArray *src);
	ElementType (*Back)(const ValArray *src);
	ElementType (*Front)(const ValArray *src);	
    int (*RemoveRange)(ValArray *src,size_t start,size_t end);
    int (*Resize)(ValArray *src, size_t newSize);
} ValArrayInterface;
