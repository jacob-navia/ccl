#include <float.h>
#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayDouble                                                  *
 ****************************************************************************/
#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayDouble
#define tagValArray tagValArrayDoubleInterface
#define ElementType double
#define ValArrayInterface ValArrayDoubleInterface
#define ValArray ValArrayDouble
#define iValArrayInterface iValArrayDouble
#define MaxElementType	DBL_MAX
#define MinElementType	DBL_MIN
static const guid ValArrayGuidDouble = {0x39a51a8, 0x2a30, 0x4c88,
{0x9d,0x81,0xf3,0x1f,0x30,0xf4,0x5c,0x85}
};
#define ValArrayGuid ValArrayGuidDouble
#include "valarraygen.c"
