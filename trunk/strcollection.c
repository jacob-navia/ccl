/*------------------------------------------------------------------------
Module:        wstrcollection.c
Author:        jacob
Project:       Containers
State:
Creation Date:
Description:   Implements the string collection container.
Added fixes from Gerome. Oct. 20 2005
------------------------------------------------------------------------*/
#include <wchar.h>
#include "containers.h"
#include "ccl_internal.h"
#define CHAR_TYPE char
#define SNPRINTF snprintf
#define STRCMP strcmp
#define STRICMP stricmp
#define STRCPY strcpy
#define STRLEN strlen
#define STRSTR strstr
#define ElementType strCollection
#define iElementType istrCollection
#define STRSTR strstr 
#define GETLINE GetLine 
#define STRCOMPAREFUNCTION StringCompareFn
#define INTERFACE_TYP strCollectionInterface
#define INTERFACE_OBJECT istrCollection

/* Definition of the String Collection type */
struct strCollection {
    strCollectionInterface *VTable; /* The table of functions */
    size_t count;                  /* in element size units */
    unsigned int Flags;             /* Read-only or other flags */
    CHAR_TYPE **contents;               /* The contents of the collection */
    size_t capacity;                /* in element_size units */
    size_t timestamp;
    ErrorFunction RaiseError;
    StringCompareFn strcompare;
    CompareInfo *StringCompareContext;
    ContainerMemoryManager *Allocator;
    DestructorFunction DestructorFn;
};

static const guid strCollectionGuid = {0x64bea19b, 0x243b, 0x487a,
{0x9a,0xd6,0xcd,0xfe,0xa9,0x37,0x6e,0x88}
};

#include "strcollectiongen.c"
