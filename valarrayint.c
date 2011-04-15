#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayInt                                                  *
 ****************************************************************************/
#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayInt
#define tagValArray tagValArrayIntInterface
#define ElementType int
#define ValArrayInterface ValArrayIntInterface
#define ValArray ValArrayInt
#define iValArrayInterface iValArrayInt
#define __IS_INTEGER__
#include "valarraygen.c"
#undef __IS_INTEGER__
