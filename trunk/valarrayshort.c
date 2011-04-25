#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayShort                                                  *
 ****************************************************************************/
#undef ElementType
#undef ValArrayShorterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayShort
#define tagValArray tagValArrayShortInterface
#define ElementType short
#define ValArrayInterface ValArrayShortInterface
#define ValArray ValArrayShort
#define iValArrayInterface iValArrayShort
#define MinElementType SHRT_MIN
#define MaxElementType SHRT_MAX
static const guid ValArrayGuidShort = {0x85a86bdd, 0xeca1, 0x4966,
{0x8d,0x1a,0x78,0x38,0x89,0x0,0x87,0xb4}
};
#define ValArrayGuid ValArrayGuidShort
#define __IS_INTEGER__
#include "valarraygen.c"
