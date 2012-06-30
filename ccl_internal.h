#ifndef __CCL_INTERNAL_H__
#define __CCL_INTERNAL_H__
#ifdef NO_C99
/* No flexible arrays */
#define MINIMUM_ARRAY_INDEX     1
#else
/* Use C99 features */
#define MINIMUM_ARRAY_INDEX
#endif
#ifdef SPARC32 // old sparcs are missing this protptype
int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
#endif

/* This macro supposes that n is a power of two */
#define roundupTo(x,n) (((x)+((n)-1))&(~((n)-1)))
#define roundup(x) roundupTo(x,sizeof(void *))
/* This function is needed to read a line from a file.
   The resulting line is allocated with the given memory manager
*/
int GetLine(char **LinePointer,int *n, FILE *stream,ContainerAllocator *mm);
int WGetLine(wchar_t **LinePointer,int *n, FILE *stream,ContainerAllocator *mm);
/* GUIDs used to mark saved container files */
typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    unsigned char Data4[8];
} guid;

/*----------------------------------------------------------------------------*/
/* Definition of the Mask type                                                */
/*----------------------------------------------------------------------------*/
struct _Mask {
    size_t length;
    const ContainerAllocator *Allocator;
    char data[MINIMUM_ARRAY_INDEX];
};

/*----------------------------------------------------------------------------*/
/* Definition of the Generic container type                                   */
/*----------------------------------------------------------------------------*/
struct _GenericContainer {
    struct tagGenericContainerInterface *vTable;
    size_t count;             /* number of elements in the container */
    unsigned int Flags;       /* Read-only or other flags */
};
typedef struct tagGenericIterator {
	Iterator it;
	GenericContainer *Gen;
} GenericIterator;

typedef struct tagSequentialIterator {
	Iterator it;
	SequentialContainer *Gen;
} SequentialIterator;


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
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
} ;

struct VectorIterator {
	Iterator it;
	Vector *AL;
	size_t index;
	unsigned timestamp;
	unsigned long Flags;
	void *Current;
	char ElementBuffer[1];
};


/*----------------------------------------------------------------------------*/
/* Definition of the bitstring type                                           */
/*----------------------------------------------------------------------------*/
struct _BitString {
    BitStringInterface *VTable; /* The table of functions */
    size_t count;                  /* number of elements in the array */
    BIT_TYPE *contents;               /* The contents of the collection */
    size_t capacity;                /* allocated space in the contents vector */
    unsigned timestamp;
    unsigned int Flags;             /* Read-only or other flags */
    const ContainerAllocator *Allocator;
} ;

/*----------------------------------------------------------------------------*/
/* Definition of the list and list element type                               */
/*----------------------------------------------------------------------------*/
struct _ListElement {
    struct _ListElement *Next;
#ifdef SPARC32
    void *alignment;
#endif
    char Data[MINIMUM_ARRAY_INDEX];
};

struct _List {
    ListInterface *VTable;      /* Methods table */
    size_t count;               /* in elements units */
    unsigned Flags;
    unsigned timestamp;         /* Changed at each modification */
    size_t ElementSize;         /* Size (in bytes) of each element */
    ListElement *Last;         /* The last item */
    ListElement *First;        /* The contents of the list start here */
    CompareFunction Compare;    /* Element comparison function */
    ErrorFunction RaiseError;   /* Error function */
    ContainerHeap *Heap;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};

struct ListIterator {
    Iterator it;
    List *L;
    size_t index;
    ListElement *Current;
    ListElement *Previous;
    unsigned  timestamp;
    char ElementBuffer[1];
};

#if 0
/*----------------------------------------------------------------------------*/
/* Definition of the stringlist and stringlist element type                   */
/*----------------------------------------------------------------------------*/
struct _StringListElement {
    struct _StringListElement *Next;
    char Data[MINIMUM_ARRAY_INDEX];
};

struct _StringList {
    iStringListInterface *VTable;      /* Methods table */
    size_t count;               /* in elements units */
    unsigned Flags;
    unsigned timestamp;         /* Changed at each modification */
    size_t ElementSize;         /* Size (in bytes) of each element */
    StringListElement *Last;         /* The last item */
    StringListElement *First;        /* The contents of the list start here */
    CompareFunction Compare;    /* Element comparison function */
    ErrorFunction RaiseError;   /* Error function */
    ContainerHeap *Heap;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};

struct StringListIterator {
    Iterator it;
    StringList *L;
    size_t index;
    StringListElement *Current;
    StringListElement *Previous;
    unsigned timestamp;
    char *ElementBuffer;
};

/*----------------------------------------------------------------------------*/
/* Definition of the wstringlist and wstringlist element type                 */
/*----------------------------------------------------------------------------*/
struct _wStringListElement {
    struct _wStringListElement *Next;
    wchar_t Data[MINIMUM_ARRAY_INDEX];
};

struct _WStringList {
    iWStringListInterface *VTable;      /* Methods table */
    size_t count;               /* in elements units */
    unsigned Flags;
    unsigned timestamp;         /* Changed at each modification */
    size_t ElementSize;         /* Size (in bytes) of each element */
    wStringListElement *Last;         /* The last item */
    wStringListElement *First;        /* The contents of the list start here */
    CompareFunction Compare;    /* Element comparison function */
    ErrorFunction RaiseError;   /* Error function */
    ContainerHeap *Heap;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};

struct WStringListIterator {
    Iterator it;
    WStringList *L;
    size_t index;
    wStringListElement *Current;
    wStringListElement *Previous;
    unsigned timestamp;
    wchar_t *ElementBuffer;
};
#endif


/*----------------------------------------------------------------------------*/
/* dlist                                                                      */
/*----------------------------------------------------------------------------*/
struct _DlistElement {
    struct _DlistElement *Next;
    struct _DlistElement *Previous;
    char Data[MINIMUM_ARRAY_INDEX];
};

struct Dlist {
    DlistInterface *VTable;
    size_t count;                    /* in elements units */
    unsigned Flags;
    unsigned timestamp;
    size_t ElementSize;
    DlistElement *Last;         /* The last item */
    DlistElement *First;        /* The contents of the Dlist start here */
    DlistElement *FreeList;
    CompareFunction Compare;     /* Element comparison function */
    ErrorFunction RaiseError;        /* Error function */
    ContainerHeap *Heap;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};

struct DListIterator {
    Iterator it;
    Dlist *L;
    size_t index;
    DlistElement *Current;
    unsigned timestamp;
    char ElementBuffer[1];
};
/*----------------------------------------------------------------------------*/
/* Dictionary                                                                 */
/*----------------------------------------------------------------------------*/
struct _Dictionary {
	DictionaryInterface *VTable;
	size_t count;
	unsigned Flags;
	size_t size;
	ErrorFunction RaiseError;
	unsigned timestamp;
	size_t ElementSize;
	const ContainerAllocator *Allocator;
	DestructorFunction DestructorFn;
	HashFunction hash;
	struct DataList {
		struct DataList *Next;
		char *Key;
		void *Value;
	} **buckets;
};
struct DictionaryIterator {
	Iterator it;
	Dictionary *Dict;
	size_t index;
	struct DataList *dl;
	unsigned timestamp;
	unsigned long Flags;
};

/*----------------------------------------------------------------------------*/
/* Wide character dictionary (key is wchar_t)                                 */
/*----------------------------------------------------------------------------*/
struct _WDictionary {
	WDictionaryInterface *VTable;
	size_t count;
	unsigned Flags;
	size_t size;
	ErrorFunction RaiseError;
	unsigned timestamp;
	size_t ElementSize;
	const ContainerAllocator *Allocator;
	DestructorFunction DestructorFn;
	WHashFunction hash;
	struct WDataList {
		struct WDataList *Next;
		wchar_t *Key;
		void *Value;
	} **buckets;
};
struct WDictionaryIterator {
	Iterator it;
	WDictionary *Dict;
	size_t index;
	struct WDataList *dl;
	unsigned timestamp;
	unsigned long Flags;
};

/*----------------------------------------------------------------------------*/
/* Hash table                                                                 */
/*----------------------------------------------------------------------------*/
typedef struct _HashEntry {
    struct _HashEntry *next;
    unsigned int       hash;
    const void        *key;
    size_t             klen;
    char              val[1];
} HashEntry  ;
/*
 * Data structure for iterating through a hash table.
 *
 * We keep a pointer to the next hash entry here to allow the current
 * hash entry to be freed or otherwise mangled between calls to
 * hash_next().
 */
typedef struct _HashIndex {
    HashTable     *ht;
    HashEntry     *This, *next;
    unsigned int   index;
} HashIndex;
/*
 * The size of the array is always a power of two. We use the maximum
 * index rather than the size so that we can use bitwise-AND for
 * modular arithmetic.
 * The count of hash entries may be greater depending on the chosen
 * collision rate.
 */
struct _HashTable {
    HashTableInterface *VTable;
    Pool          *pool;
    HashEntry   **array;
    HashIndex     iterator;  /* For hash_first(NULL, ...) */
    unsigned int   count, max;
    GeneralHashFunction   Hash;
    HashEntry     *free;  /* List of recycled entries */
    unsigned       Flags;
    ErrorFunction  RaiseError;
    unsigned       timestamp;
    size_t         ElementSize;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};

struct HashTableIterator {
	Iterator it;
	HashTable *ht;
	HashIndex hi;
	unsigned timestamp;
	unsigned long Flags;
	HashIndex *Current;
};

/*----------------------------------------------------------------------------*/
/* Tree map                                                                   */
/*----------------------------------------------------------------------------*/
/* Node in a balanced binary tree. */
struct Node {
    struct Node *up;        /* Parent (NULL for root). */
    struct Node *down[2];   /* Left child, right child. */
#ifdef SPARC32
    double alignment;
#endif
    char data[1];
};

/* A balanced binary tree. */
struct tagTreeMap {
    TreeMapInterface *VTable;
    size_t count;                /* Current node count. */
    struct Node *root;       /* Tree's root, NULL if empty. */
    CompareFunction compare;   /* To compare nodes. */
    ErrorFunction RaiseError;
    CompareInfo *aux;            /* Auxiliary data. */
    size_t ElementSize;
    size_t max_size;            /* Max size since last complete rebalance. */
    unsigned Flags;
    unsigned timestamp;
    ContainerHeap *Heap;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};

#define BST_MAX_HEIGHT 40
struct TreeMapIterator {
    Iterator it;
    TreeMap *bst_table;
    struct Node *bst_node;
    unsigned timestamp;
    size_t bst_height;
    struct Node *bst_stack[BST_MAX_HEIGHT];
    unsigned long Flags;
};

/*----------------------------------------------------------------------------*/
/* String collections                                                         */
/*----------------------------------------------------------------------------*/

/* Definition of the String Collection type */
struct strCollection {
    strCollectionInterface *VTable; /* The table of functions */
    size_t count;                  /* in element size units */
    unsigned int Flags;             /* Read-only or other flags */
    char **contents;               /* The contents of the collection */
    size_t capacity;                /* in element_size units */
    unsigned timestamp;
    ErrorFunction RaiseError;
    StringCompareFn strcompare;
    CompareInfo *StringCompareContext;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};

/* Definition of the wide string Collection type */
struct WstrCollection {
    WstrCollectionInterface *VTable; /* The table of functions */
    size_t count;                  /* in element size units */
    unsigned int Flags;             /* Read-only or other flags */
    wchar_t **contents;               /* The contents of the collection */
    size_t capacity;                /* in element_size units */
    unsigned timestamp;
    ErrorFunction RaiseError;
    StringCompareFn strcompare;
    CompareInfo *StringCompareContext;
    const ContainerAllocator *Allocator;
    DestructorFunction DestructorFn;
};
#define CCL_PRIORITY_MIN	(INT_MIN+1)
#define CCL_PRIORITY_MAX	(INT_MAX-1)
#endif
