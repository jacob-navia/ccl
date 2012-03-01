#include <limits.h>
#include <stddef.h>
#include "containers.h"
#include "ccl_internal.h"
#include "assert.h"

#define HASHFUNCTION HashFunction
#define ITERATOR DictionaryIterator
#define DATALIST DataList
#define DICTIONARY Dictionary
#define CHARTYPE char
#define INTERFACE DictionaryInterface
#define EXTERNAL_NAME iDictionary
#define STRCPY strcpy
#define STRCMP strcmp
#define STRLEN strlen
#define iSTRCOLLECTION istrCollection
#define STRCOLLECTION strCollection

#include "dictionarygen.c"
