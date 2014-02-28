#include "containers.h"
static ContainerAllocator DefaultAllocatorObject = { malloc,free,realloc,calloc};
ContainerAllocator *CurrentAllocator = &DefaultAllocatorObject;
static ContainerAllocator *SetCurrentAllocator(ContainerAllocator *in)
{
        ContainerAllocator *c;
        if (in == NULL)
                return CurrentAllocator;
        c = CurrentAllocator;
        CurrentAllocator = in;
        return c;
}

static ContainerAllocator *GetCurrentAllocator(void)
{	
	return CurrentAllocator;
}

AllocatorInterface iAllocator = {
	SetCurrentAllocator,
	GetCurrentAllocator,
};

