#include <limits.h>
#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayUInt                                                  *
 ****************************************************************************/
#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayUInt
#define tagValArray tagValArrayUIntInterface
#define ElementType unsigned
#define ValArrayInterface ValArrayUIntInterface
#define ValArray ValArrayUInt
#define iValArrayInterface iValArrayUInt
#define MaxElementType UINT_MAX
#define MinElementType 0
#define __IS_UNSIGNED__
#define __IS_INTEGER__
static const guid ValArrayGuidUInt = {0x597348e1, 0xe84, 0x4959, {0xae,0x1c,0x62,0x57,0xa8,0xa0,0xea,0x9d}};
#define ValArrayGuid ValArrayGuidUInt
#include "valarraygen.c"
