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

#include "valarraygen.c"
