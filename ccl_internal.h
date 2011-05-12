#ifdef NO_C99
/* No flexible arrays */
#define MINIMUM_ARRAY_INDEX     1
#else
/* Use C99 features */
#define MINIMUM_ARRAY_INDEX
#endif

/* This macro supposes that n is a power of two */
#define roundupTo(x,n) (((x)+((n)-1))&(~((n)-1)))
#define roundup(x) roundupTo(x,sizeof(void *))
/* This function is needed to read a line from a file.
   The resulting line is allocated with the given memory manager
*/
int GetLine(unsigned char **LinePointer,int *n, FILE *stream,ContainerMemoryManager *mm);
typedef struct {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        unsigned char Data4[8];
} guid;

struct _Mask {
        size_t length;
        ContainerMemoryManager *Allocator;
        char data[MINIMUM_ARRAY_INDEX];
};

struct _GenericContainer {
    struct tagGenericContainerInterface *vTable;
    size_t count;             /* number of elements in the container */
    unsigned int Flags;       /* Read-only or other flags */
};

/*----------------------------------------------------------------------------*/
/* Definition of the vector type                                              */
/*----------------------------------------------------------------------------*/
struct _Vector {
    VectorInterface *VTable;       /* The table of functions */
    size_t count;                  /* number of elements in the array */
    unsigned int Flags;            /* Read-only or other flags */
    size_t ElementSize;            /* Size of the elements stored in this array. */
    void *contents;                /* The contents of the collection */
    size_t capacity;               /* allocated space in the contents vector */
    unsigned timestamp;            /* Incremented at each change */
    CompareFunction CompareFn;     /* Element comparison function */
    ErrorFunction RaiseError;      /* Error function */
    ContainerMemoryManager *Allocator;
    DestructorFunction DestructorFn;
} ;

/*----------------------------------------------------------------------------*/
/* Definition of the bitstring type                                           */
/*----------------------------------------------------------------------------*/
struct _BitString {
    BitStringInterface *VTable; /* The table of functions */
    size_t count;                  /* number of elements in the array */
    BIT_TYPE *contents;               /* The contents of the collection */
    size_t capacity;                /* allocated space in the contents vector */
    unsigned long timestamp;
    unsigned int Flags;             /* Read-only or other flags */
    ContainerMemoryManager *Allocator;
} ;

/*----------------------------------------------------------------------------*/
/* Definition of the list and list element type                               */
/*----------------------------------------------------------------------------*/
typedef struct _list_element {
    struct _list_element *Next;
    char Data[MINIMUM_ARRAY_INDEX];
} list_element;

struct _List {
    ListInterface *VTable;      /* Methods table */
    size_t count;               /* in elements units */
    unsigned Flags;
    unsigned timestamp;         /* Changed at each modification */
    size_t ElementSize;         /* Size (in bytes) of each element */
    list_element *Last;         /* The last item */
    list_element *First;        /* The contents of the list start here */
    CompareFunction Compare;    /* Element comparison function */
    ErrorFunction RaiseError;   /* Error function */
    ContainerHeap *Heap;
    ContainerMemoryManager *Allocator;
    DestructorFunction DestructorFn;
};

/*----------------------------------------------------------------------------*/
/* Definition of the dlist element type used also by deque                    */
/*----------------------------------------------------------------------------*/
typedef struct _dlist_element {
    struct _dlist_element *Next;
    struct _dlist_element *Previous;
    char Data[MINIMUM_ARRAY_INDEX];
} dlist_element;

