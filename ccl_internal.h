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


