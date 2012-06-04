#include "containers.h"
#undef DATA_TYPE
#define DATA_TYPE int
#define COMPARE_EXPRESSION(A, B) ((*B)->Data > (*A)->Data ? -1 : (*B)->Data != (*A)->Data)
#include "listgen.c"
#undef DATA_TYPE
