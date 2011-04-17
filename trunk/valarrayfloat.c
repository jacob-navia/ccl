#include <float.h>
#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayFloat                                                  *
 ****************************************************************************/
#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayFloat
#define tagValArray tagValArrayFloatInterface
#define ElementType float
#define ValArrayInterface ValArrayFloatInterface
#define ValArray ValArrayFloat
#define iValArrayInterface iValArrayFloat
#define MaxElementType FLT_MAX
#define MinElementType FLT_MIN
static const guid ValArrayGuidFloat = {0xb614486e, 0x61e2, 0x45bf,
{0xb3,0xf5,0x7,0xfc,0xd3,0x55,0x77,0x9}
};
#define ValArrayGuid ValArrayGuidFloat
#include "valarraygen.c"
