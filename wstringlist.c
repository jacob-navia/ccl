#include "containers.h"
#include "ccl_internal.h"
#include <wchar.h>
#define CHARTYPE wchar_t
#define STRCPY wcscpy
#define STRLEN wcslen
#define STRCMP wcscmp
#define DATA_TYPE wString
#define StringListGuid WStringListGuid
static const guid WStringListGuid = {0xA0543963,0xCBAC,0x4AFB,
{0x09,0xC0,0xA1,0x40,0x79,0x32,0x8D,0x7B}
};

#include "stringlistgen.c"

#undef CHARTYPE
#undef STRCPY
#undef STRLEN
#undef STRCMP
#undef iSTRINGLIST
#undef INTERFACE
#undef LIST_ELEMENT
#undef LIST_TYPE
#undef LIST_TYPE_Iterator

