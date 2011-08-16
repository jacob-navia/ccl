#include "containers.h"
#include "ccl_internal.h"
#define CHARTYPE char
#define STRCPY strcpy
#define STRLEN strlen
#define iSTRINGLIST iStringListInterface
#define INTERFACE iStringList
#define LIST_ELEMENT stringlist_element
#define LIST_TYPE StringList
#define LIST_TYPEIterator StringListIterator
static const guid StringListGuid = {0x13327EA7,0x78ED,0x4FD2,
{0x95,0xED,0x1C,0x11,0x0E,0xB9,0x71,0x9C}
};

#include "stringlistgen.c"

