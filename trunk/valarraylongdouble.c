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

#include "valarraygen.c"
