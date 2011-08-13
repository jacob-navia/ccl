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
int GetLine(char **LinePointer,int *n, FILE *stream,ContainerMemoryManager *mm);
int WGetLine(wchar_t **LinePointer,int *n, FILE *stream,ContainerMemoryManager *mm);
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
    ContainerMemoryManager *Allocator;
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
    ContainerMemoryManager *Allocator;
    DestructorFunction DestructorFn;
} ;

struct VectorIterator {
	Iterator it;
	Vector *AL;
	size_t index;
	size_t timestamp;
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

struct ListIterator {
    Iterator it;
    List *L;
    size_t index;
    list_element *Current;
	list_element *Previous;
    size_t timestamp;
    char ElementBuffer[1];
};


/*----------------------------------------------------------------------------*/
/* dlist                                                                      */
/*----------------------------------------------------------------------------*/
typedef struct _dlist_element {
    struct _dlist_element *Next;
    struct _dlist_element *Previous;
    char Data[MINIMUM_ARRAY_INDEX];
} dlist_element;

struct Dlist {
    DlistInterface *VTable;
    size_t count;                    /* in elements units */
    unsigned Flags;
    unsigned timestamp;
    size_t ElementSize;
    dlist_element *Last;         /* The last item */
    dlist_element *First;        /* The contents of the Dlist start here */
    dlist_element *FreeList;
    CompareFunction Compare;     /* Element comparison function */
    ErrorFunction RaiseError;        /* Error function */
    ContainerHeap *Heap;
    ContainerMemoryManager *Allocator;
    DestructorFunction DestructorFn;
};

struct DListIterator {
    Iterator it;
    Dlist *L;
    size_t index;
    dlist_element *Current;
    size_t timestamp;
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
	ContainerMemoryManager *Allocator;
	DestructorFunction DestructorFn;
	unsigned (*hash)(const char *Key);
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
	size_t timestamp;
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
    HashEntry     *this, *next;
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
    HashFunction   Hash;
    HashEntry     *free;  /* List of recycled entries */
	unsigned       Flags;
	ErrorFunction  RaiseError;
	unsigned       timestamp;
	size_t         ElementSize;
	ContainerMemoryManager *Allocator;
	DestructorFunction DestructorFn;
};

struct HashTableIterator {
	Iterator it;
	HashTable *ht;
	HashIndex hi;
	size_t timestamp;
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
    ContainerMemoryManager *Allocator;
    DestructorFunction DestructorFn;
};

#define BST_MAX_HEIGHT 40
struct TreeMapIterator {
    Iterator it;
    TreeMap *bst_table;
    struct Node *bst_node;
    size_t timestamp;
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
    size_t timestamp;
    ErrorFunction RaiseError;
    StringCompareFn strcompare;
    CompareInfo *StringCompareContext;
    ContainerMemoryManager *Allocator;
    DestructorFunction DestructorFn;
};

/* Definition of the wide string Collection type */
struct WstrCollection {
    WstrCollectionInterface *VTable; /* The table of functions */
    size_t count;                  /* in element size units */
    unsigned int Flags;             /* Read-only or other flags */
    wchar_t **contents;               /* The contents of the collection */
    size_t capacity;                /* in element_size units */
    size_t timestamp;
    ErrorFunction RaiseError;
    StringCompareFn strcompare;
    CompareInfo *StringCompareContext;
    ContainerMemoryManager *Allocator;
    DestructorFunction DestructorFn;
};
