typedef struct _INTERNAL_NAME EXTERNAL_NAME;
typedef struct _INTERNAL_INTERFACE_NAME {
    size_t (*Size)(EXTERNAL_NAME *L);        /* Returns the number of elements stored */
    unsigned (*GetFlags)(EXTERNAL_NAME *L);                      /* Gets the flags */
    unsigned (*SetFlags)(EXTERNAL_NAME *L,unsigned flags);       /* Sets the flags */
    int (*Clear)(EXTERNAL_NAME *L);                         /* Clears all elements */
    int (*Contains)(EXTERNAL_NAME *L,CHARTYPE *element);       /* Searches an element */
    int (*Erase)(EXTERNAL_NAME *L,CHARTYPE *);       /* erases the given data if found */
    /* Frees the memory used by the collection and destroys the list */
    int (*Finalize)(EXTERNAL_NAME *L);
    /* Call a callback function with each element of the list and an extra argument */
    int (*Apply)(EXTERNAL_NAME *L,int(Applyfn)(CHARTYPE *,void *),void *arg);
    int (*Equal)(EXTERNAL_NAME *l1,EXTERNAL_NAME *l2);  /* Compares two lists (Shallow comparison) */
    EXTERNAL_NAME *(*Copy)(EXTERNAL_NAME *L);           /* Copies all items into a new list */
    ErrorFunction (*SetErrorFunction)(EXTERNAL_NAME *L,ErrorFunction); /* Set/unset the error function */
    size_t (*Sizeof)(EXTERNAL_NAME *l);
    Iterator *(*NewIterator)(EXTERNAL_NAME *L);
    int (*deleteIterator)(Iterator *);
    int (*Save)(EXTERNAL_NAME *L,FILE *stream, SaveFunction saveFn,void *arg);
    EXTERNAL_NAME *(*Load)(FILE *stream, ReadFunction loadFn,void *arg);
    size_t (*GetElementSize)(EXTERNAL_NAME *l);
 /* -------------------------------------------This is the Sequential container part */
    int (*Add)(EXTERNAL_NAME *L,CHARTYPE *newval);              /* Adds element at end */
    /* Returns the data at the given position */
    CHARTYPE *(*GetElement)(EXTERNAL_NAME *L,size_t idx);
    /* Pushes an element, using the collection as a stack */
    int (*PushFront)(EXTERNAL_NAME *L,CHARTYPE *str);
    /* Pops the first element the list */
    int (*PopFront)(EXTERNAL_NAME *L,CHARTYPE *result);
    /* Inserts a value at the given position */
    int (*InsertAt)(EXTERNAL_NAME *L,size_t idx,CHARTYPE *newval);
    int (*EraseAt)(EXTERNAL_NAME *L,size_t idx);
    /* Replaces the element at the given position with a new one */
    int (*ReplaceAt)(EXTERNAL_NAME *L,size_t idx,CHARTYPE *newval);
    /*Returns the index of the given data or -1 if not found */
    int (*IndexOf)(EXTERNAL_NAME *L,CHARTYPE *SearchedElement,void *ExtraArgs,size_t *result);
/* -------------------------------------------This is the list container part */
    int (*InsertIn)(EXTERNAL_NAME *l, size_t idx,EXTERNAL_NAME *newData);
    int  (*CopyElement)(EXTERNAL_NAME *list,size_t idx,CHARTYPE *OutBuffer);
    /*erases the string at the indicated position */
    int (*EraseRange)(EXTERNAL_NAME *L,size_t start,size_t end);
    /* Sorts the list in place */
    int (*Sort)(EXTERNAL_NAME *l);
    /* Reverses the list */
    int (*Reverse)(EXTERNAL_NAME *l);
    /* Gets an element range from the list */
    EXTERNAL_NAME *(*GetRange)(EXTERNAL_NAME *l,size_t start,size_t end);
    /* Add a list at the end of another */
    int (*Append)(EXTERNAL_NAME *l1,EXTERNAL_NAME *l2);
    /* Set the comparison function */
    CompareFunction (*SetCompareFunction)(EXTERNAL_NAME *l,CompareFunction fn);
    CompareFunction Compare;
    int (*UseHeap)(EXTERNAL_NAME *L, ContainerMemoryManager *m);
    int (*AddRange)(EXTERNAL_NAME *L, size_t n,CHARTYPE **data);
    EXTERNAL_NAME *(*Create)(void);
    EXTERNAL_NAME *(*CreateWithAllocator)(ContainerMemoryManager *allocator);
    EXTERNAL_NAME *(*Init)(EXTERNAL_NAME *aEXTERNAL_NAME);
    EXTERNAL_NAME *(*InitWithAllocator)(EXTERNAL_NAME *aEXTERNAL_NAME,ContainerMemoryManager *allocator);
    EXTERNAL_NAME *(*SetAllocator)(EXTERNAL_NAME *l, ContainerMemoryManager  *allocator);
    int (*InitIterator)(EXTERNAL_NAME *list,void *storage);
    ContainerMemoryManager *(*GetAllocator)(EXTERNAL_NAME *list);
    DestructorFunction (*SetDestructor)(EXTERNAL_NAME *v,DestructorFunction fn);
    EXTERNAL_NAME *(*InitializeWith)(size_t n,CHARTYPE **data);
    CHARTYPE *(*Back)(const EXTERNAL_NAME *l);
    CHARTYPE *(*Front)(const EXTERNAL_NAME *l);
} INTERFACE;

extern INTERFACE iSTRINGLIST;

