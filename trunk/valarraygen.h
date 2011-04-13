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
    int (*Equal)(ValArray *first,ValArray *second);
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
    ElementType (*GetElement)(ValArray *AL,size_t idx);
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
    ValArray *(*IndexIn)(ValArray *SC,ValArraySize_t *AL);
    size_t (*GetCapacity)(const ValArray *AL);
    int (*SetCapacity)(ValArray *AL,size_t newCapacity);

    CompareFunction (*SetCompareFunction)(ValArray *l,CompareFunction fn);
    int (*Sort)(ValArray *AL);
    ValArray *(*Create)(size_t startsize);
    ValArray *(*CreateWithAllocator)(size_t startsize, ContainerMemoryManager *allocator);
    ValArray *(*Init)(ValArray *AL,size_t startsize);
    int (*AddRange)(ValArray *AL,size_t n, ElementType *newvalues);
    ValArray *(*GetRange)(ValArray *AL, size_t start, size_t end);
    int (*CopyElement)(ValArray *AL,size_t idx,ElementType *outbuf);
    ElementType *(*CopyTo)(ValArray *AL);
    int (*Reverse)(ValArray *AL);
    int (*Append)(ValArray *AL1, ValArray *AL2);
    int (*Mismatch)(ValArray *a1, ValArray *a2,size_t *mismatch);
    ContainerMemoryManager *(*GetAllocator)(ValArray *AL);
    DestructorFunction (*SetDestructor)(ValArray *cb,DestructorFunction fn);
    int (*SumTo)(ValArray *left,ValArray *right);
    int (*SubtractFrom)(ValArray *left, ValArray *right);
    int (*MultiplyWith)(ValArray *left, ValArray *right);
    int (*DivideBy)(ValArray *left, ValArray *right);
    int (*SumToScalar)(ValArray *left,ElementType right);
    int (*SubtractFromScalar)(ValArray *left, ElementType right);
    int (*MultiplyWithScalar)(ValArray *left, ElementType right);
    int (*DivideByScalar)(ValArray *left, ElementType right);
    unsigned char *(*CompareEqual)(ValArray *left, ValArray *right,unsigned char *bytearray);
    unsigned char *(*CompareEqualScalar)(ValArray *left, ElementType right,unsigned char *bytearray);
    char *(*Compare)(ValArray *left, ValArray *right,char *bytearray);
    char *(*CompareScalar)(ValArray *left, ElementType right,char *bytearray);

    int (*Fill)(ValArray *dst,ElementType fillValue);
#ifdef IS_UNSIGNED
    int (*Or)(ValArray *left, ValArray *right);
    int (*And)(ValArray *left, ValArray *right);
    int (*Xor)(ValArray *left, ValArray *right);
    int (*Not)(ValArray *left);
    int (*LeftShift)(ValArray *data,int shift);
    int (*RightShift)(ValArray *data,int shift);
    int (*OrScalar)(ValArray *left, ElementType right);
    int (*AndScalar)(ValArray *left, ElementType right);
    int (*XorScalar)(ValArray *left, ElementType right);

#endif
} ValArrayInterface;


