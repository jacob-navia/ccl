#include "containers.h"
static ContainerMemoryManager DefaultMemoryManagerObject = { malloc,free,realloc,calloc};
ContainerMemoryManager *CurrentMemoryManager = &DefaultMemoryManagerObject;
ContainerMemoryManager *SetCurrentMemoryManager(ContainerMemoryManager *in)
{
        ContainerMemoryManager *c;
        if (in == NULL)
                return CurrentMemoryManager;
        c = CurrentMemoryManager;
        CurrentMemoryManager = in;
        return c;
}

