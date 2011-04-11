#include "containers.h"
#include "ccl_internal.h"
/****************************************************************************
 *          ValArrayShort                                                  *
 ****************************************************************************/
#undef ElementType
#undef ValArrayShorterface
#undef ElementType
#undef ValArray
#undef tagValArray
#undef _ValArray
#define _ValArray _ValArrayShort
#define tagValArray tagValArrayShortInterface
#define ElementType short
#define ValArrayInterface ValArrayShortInterface
#define ValArray ValArrayShort
#define iValArrayInterface iValArrayShort

#include "valarraygen.c"
