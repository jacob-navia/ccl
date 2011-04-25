/****************************************************************************
 *          ValArraySize_t                                                 *
 ****************************************************************************/
#include "containers.h"
#include "ccl_internal.h"

#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArraySize_t
#define tagValArray tagValArraySize_tInterface
#define ElementType size_t
#define ValArrayInterface ValArraySize_tInterface
#define ValArray ValArraySize_t
#define iValArrayInterface iValArraySize_t
/* This supposes size_t is unsigned */
#define MaxElementType (ElementType)-1
#define MinElementType	0
static const guid ValArrayGuidSize_t = {0x39058047, 0x42ec, 0x4c0e,
{0xb3,0xb7,0xc,0x50,0x38,0xcd,0x24,0x8}
};
#define ValArrayGuid ValArrayGuidSize_t
#define __IS_UNSIGNED__
#define __IS_INTEGER__
#include "valarraygen.c"
