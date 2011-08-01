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
#define STRCPY wcscpy
#define STRLEN wcslen
#define _TCHAR(a) L##a
#define ElementType WstrCollection
#define iElementType iWstrCollection
#define STRSTR wcsstr 
#define GETLINE WGetLine 
#define STRCOMPAREFUNCTION WStringCompareFn
#define INTERFACE_TYP WstrCollectionInterface
#define INTERFACE_OBJECT iWstrCollection
#define NEWLINE L'\n'


static const guid strCollectionGuid = {0x64bea19b, 0x243b, 0x487a,
{0x9a,0xd6,0xcd,0xfe,0xa9,0x37,0x6e,0x89}
};

#include "strcollectiongen.c"
