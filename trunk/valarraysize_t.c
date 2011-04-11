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

#include "valarraygen.c"
