#include <wchar.h>
#define CHARTYPE wchar_t
#define INTERFACE iWStringListInterface
#define _INTERNAL_NAME _WStringList
#define EXTERNAL_NAME WStringList
#define _INTERNAL_INTERFACE_NAME tagWStringListInterface
#define iSTRINGLIST iWStringList
#ifndef LIST_ELEMENT
#define LIST_ELEMENT wStringListElement
#endif

#include "stringlistgen.h"

#undef CHARTYPE
#undef INTERFACE
#undef _INTERNAL_NAME
#undef EXTERNAL_NAME
#undef _INTERNAL_INTERFACE_NAME
#undef iSTRINGLIST
#undef LIST_ELEMENT
