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

#include "valarraygen.c"
