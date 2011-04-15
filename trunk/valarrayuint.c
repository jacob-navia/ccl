#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayUInt                                                  *
 ****************************************************************************/
#undef ElementType
#undef ValArrayInterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayUInt
#define tagValArray tagValArrayUIntInterface
#define ElementType unsigned int
#define ValArrayInterface ValArrayUIntInterface
#define ValArray ValArrayUInt
#define iValArrayInterface iValArrayUInt
#define __IS_UNSIGNED__
#include "valarraygen.c"
#undef __IS_UNSIGNED__
