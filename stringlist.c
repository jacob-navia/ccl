#include "containers.h"
#include "ccl_internal.h"
#undef CHARTYPE
#undef DATA_TYPE

#define CHARTYPE char
#define STRCPY strcpy
#define STRLEN strlen
#define STRCMP strcmp
#define DATA_TYPE String
static const guid StringListGuid = {0x13327EA7,0x78ED,0x4FD2,
{0x95,0xED,0x1C,0x11,0x0E,0xB9,0x71,0x9C}
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
