#include "containers.h"
#undef DATA_TYPE
typedef long long longlong;
#define DATA_TYPE longlong
#include "listgen.c"
#undef DATA_TYPE
