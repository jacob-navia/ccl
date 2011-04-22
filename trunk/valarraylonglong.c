#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayLLong                                                *
 ****************************************************************************/
#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayLLong
#define tagValArray tagValArrayLLongInterface
#define ElementType long long
#define ValArrayInterface ValArrayLLongInterface
#define ValArray ValArrayLLong
#define iValArrayInterface iValArrayLLong
#define __IS_INTEGER__
#define MaxElementType LLONG_MAX
#define MinElementType	LLONG_MIN
static const guid ValArrayGuidLLong = {0xe4386077, 0x540f, 0x4ec0, {0xac,0x95,0xa0,0x96,0x10,0x98,0xc4,0x7f}};
#define ValArrayGuid ValArrayGuidLLong
#include "valarraygen.c"
#undef __IS_INTEGER__
