#include "containers.h"
#include "ccl_internal.h"

int main(void)
{
    printf("%zu %zu\n",
           sizeof(struct INTERFACE_STRUCT_INTERNAL_NAME(int)),
           sizeof(struct ITERATOR(int)));
    return 0;
}
