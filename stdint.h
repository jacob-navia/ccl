#ifndef __stdint_h__
#define __stdint_h__
typedef char int8_t;
typedef unsigned char uint8_t;
#define INT8_C(a) a
#define UINT8_C(a) (a##U)
typedef short int16_t;
typedef unsigned short uint16_t;
#define INT16_C(a) a
#define UINT16_C(a) (a##U)
typedef int int32_t;
#define INT32_C(a) (a)
typedef unsigned int uint32_t;
#define UINT32_C(a) (a##U)
typedef long long int64_t;
#define INT64_C(a) (a##LL)
typedef unsigned long long uint64_t;
#define UINT64_C(a) (a##ULL)

typedef char int_least8_t;
typedef unsigned char uint_least8_t;
typedef short int_least16_t;
typedef unsigned short uint_least16_t;
typedef int int_least32_t;
typedef unsigned int uint_least32_t;
typedef long long int_least64_t;
typedef unsigned long long uint_least64_t;

#define INTMAX_C(a) (a##LL)
#define UINTMAX_C(a) (a##ULL)

typedef char int_fast8_t;
typedef unsigned char uint_fast8_t;
typedef short int_fast16_t;
typedef unsigned short uint_fast16_t;
typedef int int_fast32_t;
typedef unsigned int uint_fast32_t;
typedef long long int_fast64_t;
typedef unsigned long long uint_fast64_t;

typedef int intptr_t;
typedef unsigned int uintptr_t;
typedef long long intmax_t;
typedef unsigned long long uintmax_t;

#define INT8_MIN	(-128)
#define INT16_MIN	(-32768)
#define INT32_MIN	(-2147483648)
#define INT64_MIN	(-9223372036854775808LL)


#define INT8_MAX	127
#define INT16_MAX	32767
#define INT32_MAX	2147483647
#define INT64_MAX	9223372036854775807LL

#define UINT8_MAX 255
#define UINT16_MAX 65535
#ifndef UINT32_MAX
#define UINT32_MAX 4294967295
#endif
#define UINT64_MAX 18446744073709551615ULL

#define INT_LEAST8_MIN (-128)
#define INT_LEAST16_MIN (-32768)
#define INT_LEAST32_MIN (-2147483648)
#define INT_LEAST64_MIN (-9223372036854775808LL)

#define INT_LEAST8_MAX 127
#define INT_LEAST16_MAX 32767
#define INT_LEAST32_MAX 2147483647
#define INT_LEAST64_MAX 9223372036854775807LL

#define UINT_LEAST8_MAX 255
#define UINT_LEAST16_MAX 65535
#define UINT_LEAST32_MAX 4294967295
#define UINT_LEAST64_MAX 18446744073709551615ULL

#define INT_FAST8_MIN (-128)
#define INT_FAST16_MIN (-32768)
#define INT_FAST32_MIN (-2147483648)
#define INT_FAST64_MIN (-9223372036854775808LL)

#define INT_FAST8_MAX 127
#define INT_FAST16_MAX 32767
#define INT_FAST32_MAX 2147483647
#define INT_FAST64_MAX 9223372036854775807LL

#define UINT_FAST8_MAX 255
#define UINT_FAST16_MAX 65535
#define UINT_FAST32_MAX 4294967295
#define UINT_FAST64_MAX 18446744073709551615LL

#define INTPTR_MIN	INT32_MIN
#define INTPTR_MAX	INT32_MAX
#define UINTPTR_MAX	UINT32_MAX

#define INTMAX_MIN	INT64_MIN
#define INTMAX_MAX	INT64_MAX
#define	UINTMAX_MAX	UINT64_MAX

#ifndef SIZE_MAX
#define SIZE_MAX	0xffffffff
#endif
#define PTRDIFF_MIN	INT32_MIN
#define PTRDIFF_MAX	INT32_MAX

#ifndef RSIZE_MAX
#define RSIZE_MAX	INT32_MAX
#endif

#endif
