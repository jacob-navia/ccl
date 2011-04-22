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
static const guid ValArrayGuidULLong = {0xe4386077, 0x540f, 0x4ec0, {0xac,0x95,0xa0,0x96,0x10,0x98,0xc4,0x7f}};
#define ValArrayGuid ValArrayGuidULLong
#define __IS_UNSIGNED__
#include "valarraygen.c"
#undef __IS_INTEGER__
