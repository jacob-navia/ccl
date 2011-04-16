#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayLongLong                                                *
 ****************************************************************************/
#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayLongLong
#define tagValArray tagValArrayLongLongInterface
#define ElementType long long
#define ValArrayInterface ValArrayLongLongInterface
#define ValArray ValArrayLongLong
#define iValArrayInterface iValArrayLongLong
#define __IS_INTEGER__
static const guid ValArrayGuidInt = {0xe4386077, 0x540f, 0x4ec0, {0xac,0x95,0xa0,0x96,0x10,0x98,0xc4,0x7f}};
#define ValArrayGuid ValArrayGuidInt
#include "valarraygen.c"
#undef __IS_INTEGER__
