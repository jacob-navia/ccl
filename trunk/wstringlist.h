#include <wchar.h>
#undef CHARTYPE
#undef DATA_TYPE

#define DATA_TYPE wString
#define CHARTYPE wchar_t
#include "stringlistgen.h"
