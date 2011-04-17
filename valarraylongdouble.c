#include <float.h>
#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayLongDouble                                                  *
 ****************************************************************************/
#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayLongDouble
#define tagValArray tagValArrayLongDoubleInterface
#define ElementType long double
#define ValArrayInterface ValArrayLongDoubleInterface
#define ValArray ValArrayLongDouble
#define iValArrayInterface iValArrayLongDouble
#define MaxElementType	LDBL_MAX
#define MinElementType  LDBL_MIN
static const guid ValArrayGuidLongDouble = {0xd0c2db6e, 0x2120, 0x40e8,
{0xa0,0x33,0x4a,0x1e,0xd7,0x12,0x32,0x80}
};
#define ValArrayGuid ValArrayGuidLongDouble
#include "valarraygen.c"
