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
#define WCHAR_TYPE
#define CHAR_TYPE wchar_t
#define SNPRINTF swprintf
#define STRCMP wcscmp
#define STRICMP wcscasecmp
#define STRCPY wcscmp
#define STRLEN wcslen
#define _TCHAR(a) L##a
#define ElementType WStringCollection
#define iElementType iWStringCollection
#define STRSTR wcsstr 
#define GETLINE WGetLine 
#define STRCOMPAREFUNCTION WStringCompareFn
#define INTERFACE_TYP WStringCollectionInterface
#define INTERFACE_OBJECT iWStringCollection
/* Definition of the String Collection type */
struct WStringCollection {
    WStringCollectionInterface *VTable; /* The table of functions */
    size_t count;                  /* in element size units */
    unsigned int Flags;             /* Read-only or other flags */
    CHAR_TYPE **contents;               /* The contents of the collection */
    size_t capacity;                /* in element_size units */
	size_t timestamp;
	ErrorFunction RaiseError;
	WStringCompareFn strcompare;
	CompareInfo *StringCompareContext;
    ContainerMemoryManager *Allocator;
	DestructorFunction DestructorFn;
};

static const guid StringCollectionGuid = {0x64bea19b, 0x243b, 0x487a,
{0x9a,0xd6,0xcd,0xfe,0xa9,0x37,0x6e,0x89}
};

#include "strcollectiongen.c"
