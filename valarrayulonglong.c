#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayULLong                                                *
 ****************************************************************************/
#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayULLong
#define tagValArray tagValArrayULLongInterface
#define ElementType unsigned long long
#define ValArrayInterface ValArrayULLongInterface
#define ValArray ValArrayULLong
#define iValArrayInterface iValArrayULLong
#define __IS_INTEGER__
#define MaxElementType ULLONG_MAX
#define MinElementType	0
static const guid ValArrayGuidULLong = {0x209022f8, 0x4f50, 0x4437, {0x93,0xb1,0xe6,0xc5,0xf5,0x6f,0x47,0xde}
};

#define ValArrayGuid ValArrayGuidULLong
#define __IS_UNSIGNED__
#include "valarraygen.c"
#undef __IS_INTEGER__
