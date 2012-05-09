#include "containers.h"
static ContainerAllocator DefaultAllocatorObject = { malloc,free,realloc,calloc};
ContainerAllocator *CurrentAllocator = &DefaultAllocatorObject;
ContainerAllocator *SetCurrentAllocator(ContainerAllocator *in)
{
        ContainerAllocator *c;
        if (in == NULL)
                return CurrentAllocator;
        c = CurrentAllocator;
        CurrentAllocator = in;
        return c;
}

