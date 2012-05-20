#include <limits.h>
#include <stddef.h>
#include "containers.h"
#include "ccl_internal.h"
#include "assert.h"
#undef ITERATOR
#undef INTERFACE
#undef DATA_TYPE

#define HASHFUNCTION WHashFunction
#define ITERATOR WDictionaryIterator
#define CHARTYPE wchar_t
#define DATALIST WDataList
#define DATA_TYPE WDictionary
#define INTERFACE WDictionaryInterface
#define EXTERNAL_NAME iWDictionary
#define STRCPY wcscpy
#define STRCMP wcscmp
#define STRLEN wcslen
#define iSTRCOLLECTION iWstrCollection
#define STRCOLLECTION WstrCollection

#include "dictionarygen.c"
