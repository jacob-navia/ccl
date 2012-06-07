#include "containers.h"
#undef DATA_TYPE
typedef long long longlong;
#define DATA_TYPE longlong
#define COMPARE_EXPRESSION(A, B) (B > A ? -1 : B != A)
#include "dlistgen.c"
#undef DATA_TYPE
