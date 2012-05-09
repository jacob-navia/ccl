/*
The bloom filter uses k hash functions to store k bits in a bit string that
represent a stored value. It can return false positives, but if it says that
an element is NOT there, it is definitely not there.

The size of the bloom filter and the number of hash functions you should be
using depending on your application can be calculated using the formulas on
the Wikipedia page:

m = -n*ln(p)/(ln(2)^2)

This will tell you the number of bits m to use for your filter, given the
number n of elements in your filter and the false positive probability p you
want to achieve. All that for the ideal number of hash functions k which you
can calculate like this:

k = 0.7*m/n

*/
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "containers.h"
struct tagBloomFilter {
	size_t count; /* Elements stored already */
	size_t MaxNbOfElements;
	size_t HashFunctions;
	size_t nbOfBits;
	ContainerAllocator *Allocator;
	unsigned char *bits;
	unsigned Seeds[1];
};

/*-----------------------------------------------------------------------------
   MurmurHash2, by Austin Appleby
   Note - This code makes a few assumptions about how your machine behaves -
   1. We can read a 4-byte value from any address without crashing
   2. sizeof(int) == 4
   And it has a few limitations -
   1. It will not work incrementally.
   2. It will not produce the same results on little-endian and big-endian
      machines.
*/
static size_t Hash(const void * key, size_t len, unsigned int seed )
{
	/* 'm' and 'r' are mixing constants generated offline.
	   They're not really 'magic', they just happen to work well.
	*/

	const unsigned int m = 0x5bd1e995;
	const int r = 24;
	/* Initialize the hash to a 'random' value */
	size_t h = seed ^ len;
	/* Mix 4 bytes at a time into the hash */
	const unsigned char * data = key;
	while(len >= 4)	{
		unsigned int k = *(unsigned int *)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;	len -= 4;
	}
	/* Handle the last few bytes of the input array */
	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
	        h *= m;
	};
	/* Do a few final mixes of the hash to ensure the last few
	   bytes are well-incorporated. */
	h ^= h >> 13;	h *= m; h ^= h >> 15;
	return h;
}
#define TWICE_LOG_2 1.386294361119890618834464242916353136151000268720
#ifdef _MSC_VER
static long round(double d)
{
	if (d > 0) return (long)(d+0.5);
	return (long)(d-0.5);
}
#endif
static BloomFilter *Create(size_t nbOfElements,double Probability)
{
	size_t nbOfBits;
	size_t k;
	BloomFilter *result;

	if (Probability >= 1.0 || Probability <= 0.0) {
		iError.RaiseError("BloomFilter.Create",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	nbOfBits = -round(nbOfElements*log(Probability)/TWICE_LOG_2);
	k = round(0.7*nbOfBits/nbOfElements);
	result = CurrentAllocator->malloc(sizeof(BloomFilter) + k*sizeof(int));
	if (result == NULL) {
		goto errMem;
	}
	memset(result,0,sizeof(*result));
	result->bits = CurrentAllocator->malloc(1+nbOfBits/8);
	if (result->bits == NULL) {
		CurrentAllocator->free(result);
errMem:
		iError.RaiseError("BloomFilter.Create",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	memset(result->bits,0,1+nbOfBits/8);
	result->nbOfBits = nbOfBits;
	result->MaxNbOfElements = nbOfElements;
	result->HashFunctions = k;
	while (k > 0) {
		k--;
		result->Seeds[k] = rand();

	}
	result->Allocator = CurrentAllocator;
	return result;
}

static size_t CalculateSpace(size_t nbOfElements,double Probability)
{
	size_t nbOfBits;
	size_t k,result;
	
	if (Probability >= 1.0 || Probability <= 0.0) {
		iError.RaiseError("BloomFilter.CalculateSpace",CONTAINER_ERROR_BADARG);
		return 0;
	}
	nbOfBits = -round(nbOfElements*log(Probability)/TWICE_LOG_2);
	k = round(0.7*nbOfBits/nbOfElements);
	result = (sizeof(BloomFilter) + k*sizeof(int));
	result += (1+nbOfBits/8);
	return result;
}

static size_t Add(BloomFilter *b, const void *key, size_t keylen)
{
	size_t hash;
	size_t i;

	if (b->MaxNbOfElements <= b->count) {
		iError.RaiseError("BloomFilter.Add",CONTAINER_FULL);
		return 0;
	}
	for (i=0; i<b->HashFunctions;i++) {
		hash = Hash(key,keylen,b->Seeds[i]);
		hash %= b->nbOfBits;
		b->bits[hash >> 3] |= 1 << (hash&7);
	}
	return ++b->count;
}

static int Find(BloomFilter *b, const void *key, size_t keylen)
{
	size_t hash;
	size_t i;

	if (b == NULL || key== NULL || keylen == 0) {
		iError.RaiseError("iBloomFilter.Find",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	for (i=0; i<b->HashFunctions;i++) {
		hash = Hash(key,keylen,b->Seeds[i]);
		hash %= b->nbOfBits;
		if ((b->bits[hash >> 3] & (1 << (hash&7))) == 0)
			return 0;
	}
	return 1;
}

static int Clear(BloomFilter *b)
{
	if (b == NULL) {
		iError.RaiseError("iBloomFilter.Find",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	memset(b->bits,0,1+b->nbOfBits/8);
	return 1;
}

static int Finalize(BloomFilter *b)
{
	if (b == NULL)
		return CONTAINER_ERROR_BADARG;
	b->Allocator->free(b->bits);
	b->Allocator->free(b);
	return 1;

}


BloomFilterInterface iBloomFilter = {
CalculateSpace,
Create,
Add,
Find,
Clear,
Finalize,
};
