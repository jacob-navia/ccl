#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "containers.h"
#include "ccl_internal.h"

#ifndef CHUNK_SIZE
#define CHUNK_SIZE 256
#endif

static unsigned char mask[] = {
	255,127,63,31,15,7,3,1};
static unsigned char maskI[]= {
	255,1,3,7,15,31,63,127};
static unsigned char BitIndexMask[] = {
	1,2,4,8,16,32,64,128};
#if 0 // CHAR_BIT is 16
static unsigned char mask[] = {
	65535,32767,16383,8191,4095,2047,1023,511,255,127,63,31,15,7,3,1};
static unsigned char maskI[]= {
	65535,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767};
static unsigned char BitIndexMask[] = {
	1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
static unsigned char InvertedBitIndexMask[] ={
	~1,~2,~4,~8,~16,~32,~64,~128,~256,~512,~1024,~2048,~4096,~8192,~16384,~32768};
#endif

#define BYTES_FROM_BITS(bitcount) (1+(bitcount)/CHAR_BIT)
#define BITPOS(idx) (idx & (CHAR_BIT-1))
static int ShiftLeftByOne(unsigned char *p, size_t z,unsigned carry);
static int Add(BitString *b,int newval);

static int NullPtrError(const char *fnName)
{
	char buf[512];

	snprintf(buf,sizeof(buf),"iBitString.%s",fnName);
	iError.RaiseError(buf,CONTAINER_ERROR_BADARG);
	return CONTAINER_ERROR_BADARG;
}

static int doerrorCall(const char *fnName,int code)
{
	char buf[256];

	snprintf(buf,sizeof(buf),"iBitString.%s",fnName);
	iError.RaiseError(buf,code);
	return code;
}
static int ReadOnlyError(const char *fnName)
{
	return doerrorCall(fnName,CONTAINER_ERROR_READONLY);
}


static size_t bytesizeFromBitLen(size_t bitlen){
	size_t bytesiz = BYTES_FROM_BITS(bitlen);
	bytesiz = (3+bytesiz) & ~3;
	return bytesiz;
}


/*------------------------------------------------------------------------
Procedure:     SetCapacity ID:1
Purpose:       Resizes a bit string without touching the data
Input:         The bitstring to be resized, and the new size in
bits
Output:        1 means no errors, 0 means no memory available for
the operation.
Errors:
------------------------------------------------------------------------*/
static int SetCapacity(BitString *b,size_t bitlen)
{
	size_t bytesize,copy_size;
	unsigned char *oldData;
	if (b == NULL)		return NullPtrError("SetCapacity");
	bytesize = bytesizeFromBitLen(bitlen);
	oldData = b->contents;
	b->contents = b->Allocator->malloc(bytesize);
	if (b->contents == NULL) {
		b->contents = oldData;
		return CONTAINER_ERROR_NOMEMORY;
	}
	memset(b->contents,0,bytesize);
	copy_size = 1+BYTES_FROM_BITS(b->count);
	copy_size = (copy_size > bytesize) ? bytesize : copy_size;
	memcpy(b->contents,oldData,copy_size);
	b->capacity = (unsigned)bytesize;
	b->Allocator->free(oldData);
	return 1;
}

static size_t GetCapacity(BitString *b)
{
	if (b == NULL) return NullPtrError("GetCapacity");
	return b->capacity;
}

static int Clear(BitString *b)
{
	if (b == NULL) return NullPtrError("Clear");
	if (b->Flags&CONTAINER_READONLY)
		return ReadOnlyError("Clear");
	b->count = 0;
	return 1;
}

static int Finalize(BitString *b)
{
	if (b == NULL) return CONTAINER_ERROR_BADARG;
	if (b->Flags&CONTAINER_READONLY)
		return ReadOnlyError("Finalize");
	b->Allocator->free(b->contents);
	b->Allocator->free(b);
	return 1;
}
static BitString *Create(size_t bitlen);
static BitString *Copy(BitString *b)
{
	BitString *result;

	if (b == NULL)	{
		NullPtrError("Copy");
		return NULL;
	}
	result = Create(b->count);
	if (result == NULL)		return NULL;
	memcpy(result->contents,b->contents,1+BYTES_FROM_BITS(b->count));
	return result;
}

static size_t GetCount(BitString *b)
{
	return (b == NULL) ? 0 : b->count;
}


static int SetElement(BitString *bs,size_t position,int b)
{
	if (bs == NULL)
		return NullPtrError("SetElement");

	if (bs->Flags&CONTAINER_READONLY)
		return ReadOnlyError("SetElement");

	if (position >= bs->count) {
		iError.RaiseError("SetElement",CONTAINER_ERROR_INDEX,bs,position);
		return CONTAINER_ERROR_INDEX;
	}
	if (b) {
		bs->contents[position >> 3] |= 1 << (position&7);
	}
	else
	    bs->contents[position >> 3] &= ~(1 << (position&7));
	return b;
}

static int GetElement(BitString *bs,size_t position)
{
	if (bs == NULL)
		return NullPtrError("GetElement");
	if (position >= bs->count)
		return 0;
	return (bs->contents[position>>3] &BitIndexMask[position&(CHAR_BIT-1)]) ? 1 : 0;
}

static int BitRightShift(BitString *bs,size_t shift)
{
	size_t left = 0,tmp,tmp1;
	size_t len,bytesize;
	unsigned char *pdata;

	if (bs == NULL) {
		return NullPtrError("BitRightShift");
	}
	len = bs->count;
	pdata = bs->contents;
	bytesize = BYTES_FROM_BITS(len);
	if (shift > len) {
		memset(bs->contents,0,bytesize);
		return 1;
	}
	if (shift >= CHAR_BIT) {
		left = shift >> 3;
		memmove(pdata,pdata+left,bytesize-left);
		shift &= 7;
		memset(pdata+bytesize-left,0,left);
	}
	if (shift) {
		while (len >= 8) {
			tmp = *pdata;
			tmp >>= shift;
			tmp1 = pdata[1];
			tmp1 <<= (CHAR_BIT - shift);
			tmp |= tmp1;
			*pdata = (unsigned char)tmp;
			pdata++;
			len -= 8;
		}
		if (len) {
			tmp = *pdata;
			tmp >>= shift;
			*pdata = (unsigned char)tmp;
		}
	}
	return 1;
}

static int BitLeftShift(BitString *bs,size_t shift){
	unsigned int tmp=0,tmp1;
	size_t left=0;
	unsigned char *pdata;
	size_t len,bytesize;

	if (bs == NULL) {
		return NullPtrError("BitLeftShift");
	}
	len = bs->count;
	pdata = bs->contents;
	bytesize = BYTES_FROM_BITS(len);
	if (shift > len) {
		memset(bs->contents,0,bytesize);
		return 1;
	}
	if (shift >= CHAR_BIT) {
		left = shift/CHAR_BIT;
		memmove(pdata+left,pdata,bytesize-left);
		shift &= (CHAR_BIT-1);
		memset(pdata,0,left);
	}
	if (shift) {
		while (len >= CHAR_BIT) {
			tmp1 = *pdata;
			*pdata = (unsigned char)((*pdata << shift) | tmp);
			len -= CHAR_BIT;
			tmp = (tmp1 >> (CHAR_BIT-shift));
			pdata++;
		}
		if (len) {
			*pdata = (unsigned char)((*pdata << shift) | tmp);
		}
	}
	return 1;
}

static BitString *GetRange(BitString *bs,size_t start,size_t end)
{
	size_t len;
	BitString *result;
	size_t startbyte,endbyte,shiftamount,bytesToCopy,idx;

	if (bs == NULL || start >=bs->count || start > end) {
		NullPtrError("GetRange");
		return NULL;
	}
	if (end >= bs->count)
		end = bs->count;
	len  = end-start;
	result = Create(len);
	if (len == 0)
		return result;
	result->count = len;
	startbyte = start/CHAR_BIT;
	endbyte = end/CHAR_BIT;
	shiftamount = start&(CHAR_BIT-1);
	if (shiftamount == 0) {
		/* Optimize this case. We can do just a memory move */
		memmove(result->contents,bs->contents+startbyte,1+endbyte-startbyte);
	}
	else {
		/* Copy the first byte. Bring the first bit to be copied into
		   the position zero by shifting right all bits smaller than
		   the start index */
		result->contents[0] = (bs->contents[startbyte] >> shiftamount);
		bytesToCopy = (endbyte-startbyte);
		idx = 1;
		while (bytesToCopy) {
			/* Put the low bits of the next byte into the correct
			   position of the last byte of the result */
			unsigned b = (bs->contents[++startbyte] << (CHAR_BIT-shiftamount));
			result->contents[idx-1] |= b;
			/* Put the high bits now in the low position of the result */
			b = (bs->contents[startbyte] >> shiftamount);
			result->contents[idx] |= b;
			bytesToCopy--;
			idx++;
		}
	}
	return result;
}

static int LessEqual(BitString *bsl,BitString *bsr){
	unsigned int *pdataL, *pdataR;
	size_t i,j,k,l;

	if ((bsl->count == 0 && bsr->count != 0) ||
		    (bsl->count == bsr->count && bsr->count == 0))
		return 1;
	if (bsr->count == 0 && bsl->count != 0)
		return 0;
	pdataL = (unsigned int *)bsl->contents;
	pdataR = (unsigned int *)bsr->contents;
	i = bytesizeFromBitLen(bsl->count) >> 3;
	j = bytesizeFromBitLen(bsr->count) >> 3;
	l = k = (i < j) ? i : j;
	while (k > 0) {
		k--;
		if (pdataL[k] &  ~pdataR[k])
			return 0;
	}
	if (i == j)		return 1;
	/* The rest must be zero */
	k = (i < j) ? j : i;
	pdataL = (i < j) ? pdataR : pdataL;
	while (l < k) {
		if (pdataL[l])
			return 0;
		l++;
	}
	return 1;
}

static int Equal(BitString *bsl,BitString *bsr)
{
	unsigned int *pdataL,*pdataR;
	size_t top;

	if (bsl == bsr)
		return 1;
	if (bsl->Flags != bsr->Flags)
		return 0;
	if (bsl == NULL || bsr == NULL)
		return 0;
	if (bsl->count != bsr->count)		return 0;
	pdataL = (unsigned int *)bsl->contents;
	pdataR = (unsigned int *)bsr->contents;
	top = BYTES_FROM_BITS(bsl->count);
	if (memcmp(pdataL,pdataR,top))		return 0;
	if (bsl->count&(CHAR_BIT-1)) {
		if ( (pdataL[top]&maskI[bsl->count&(CHAR_BIT-1)]) != (pdataR[top]&maskI[bsl->count&(CHAR_BIT-1)]) )
			return 0;
	}
	return 1;
}

static BitString * Or(BitString *bsl,BitString *bsr)
{
	size_t blen,i,resultlen;
	BitString *result;

	if (bsl == NULL || bsr == NULL) {
		NullPtrError("Or");
		return NULL;
	}
	resultlen = (bsl->count < bsr->count) ? bsr->count : bsl->count;
	result = Create(resultlen);
	blen = BYTES_FROM_BITS(resultlen);
	for (i=0; i<blen;i++) {
		result->contents[i] = bsl->contents[i] | bsr->contents[i];
	}
	result->count = resultlen;
	return result;
}

static int OrAssign(BitString *bsl,BitString *bsr)
{
	size_t len,i,blen;

	if (bsl == NULL || bsr == NULL) {
		return NullPtrError("OrAssign");
	}
	len = (bsl->count < bsr->count) ? bsl->count : bsr->count;
	blen = BYTES_FROM_BITS(len);
	for (i=0; i<blen;i++) {
		bsl->contents[i] |=  bsr->contents[i];
	}
	return 1;
}

static BitString * And(BitString *bsl,BitString *bsr)
{
	size_t len,bytelen,i,resultlen;
	BitString *result;

	if (bsl == NULL || bsr == NULL) {
		NullPtrError("And");
		return NULL;
	}
	len = (bsl->count < bsr->count) ? bsl->count : bsr->count;
	resultlen = (bsl->count < bsr->count) ? bsr->count : bsl->count;
	result = Create(resultlen);
	bytelen = BYTES_FROM_BITS(len);
	for (i=0; i<bytelen;i++) {
		result->contents[i] = bsl->contents[i] & bsr->contents[i];
	}
	if (len&(CHAR_BIT-1)) {
		unsigned int tmp = bsl->contents[i] & bsr->contents[i];
		result->contents[i] = tmp & maskI[len&(CHAR_BIT-1)];
	}
	result->count = resultlen;
	return result;
}
static int AndAssign(BitString *bsl,BitString *bsr)
{
	size_t len,i;

	if (bsl == NULL || bsr == NULL) {
		return NullPtrError("AndAssign");
	}
	len = (bsl->count < bsr->count) ? bsl->count : bsr->count;
	len = BYTES_FROM_BITS(len);
	for (i=0; i<len;i++) {
		bsl->contents[i] &= bsr->contents[i];
	}
	if (len&(CHAR_BIT-1)) {
		unsigned int tmp = bsl->contents[i] & bsr->contents[i];
		bsl->contents[i] = tmp & maskI[len&(CHAR_BIT-1)];
		i++;
	}
	while (i < 1+BYTES_FROM_BITS(bsl->count)) {
		bsl->contents[i] = 0;
		i++;
	}
	return 1;
}

static BitString * Xor(BitString *bsl,BitString *bsr)
{
	size_t len,i,resultlen;
	BitString *result;

	if (bsl == NULL || bsr == NULL) {
		NullPtrError("Xor");
		return NULL;
	}
	len = bsl->count < bsr->count ? bsl->count : bsr->count;
	resultlen = bsl->count < bsr->count ? bsr->count : bsl->count;
	result = Create(resultlen);
	len = 1+(len >>3);
	for (i=0; i<len;i++) {
		result->contents[i] = bsl->contents[i] ^ bsr->contents[i];
	}
	result->count = resultlen;
	return result;
}
static int XorAssign(BitString *bsl,BitString *bsr)
{
	size_t len,i;

	if (bsl == NULL || bsr == NULL) {
		return NullPtrError("XorAssign");
	}

	len = bsl->count < bsr->count ? bsl->count : bsr->count;
	len = 1+(len >> 3);
	for (i=0; i<len;i++) {
		bsl->contents[i] ^= bsr->contents[i];
	}
	return 1;
}

static BitString * Not(BitString *bsl)
{
	size_t len,i;
	BitString *result;

	if (bsl == NULL) {
		NullPtrError("Not");
		return NULL;
	}
	len = bsl->count;
	result = Create(len);
	len = 1+(len >> 3);
	for (i=0; i<len;i++) {
		result->contents[i] = ~bsl->contents[i];
	}
	result->count = len;
	return result;
}
static int NotAssign(BitString *bsl)
{
	size_t len,i;

	if (bsl == NULL) {
		return NullPtrError("NotAssign");
	}
	len = 1+(bsl->count>> 3);
	for (i=0; i<len;i++) {
		bsl->contents[i] = ~bsl->contents[i];
	}
	return 1;
}


static BitString *InitializeWith(size_t siz,void *p)
{
	BitString *result = Create(CHAR_BIT*siz);
	memcpy(result->contents,p,siz);
	result->count = CHAR_BIT*siz;
	return result;
}

static int AddRange(BitString *b, size_t bitSize, void *pdata)
{
	size_t i,currentByte,idx;
	unsigned char *data = pdata;
	unsigned toShift,byte;
	if (bitSize == 0) return 0;
	if (b == NULL || data == NULL) {
		return NullPtrError("AddRange");
	}
	if (SetCapacity(b,b->count+bitSize) < 0)
		return CONTAINER_ERROR_NOMEMORY;
	/* Add first byte */
	i = b->count & 7;
	toShift = (unsigned)i;
	currentByte = 0;
	if (i) {
		byte = data[currentByte++];
		while (i > 0 && bitSize > 0) {
			Add(b,byte&1);
			byte >>= 1;
			i--;
			bitSize--;
		}
	}
	if (bitSize == 0) return 1;
	idx = 1+ (b->count>>3);
	while (bitSize >= CHAR_BIT) {
		byte = data[currentByte++] << toShift;
		byte |= (data[currentByte] << (CHAR_BIT-toShift));
		b->contents[idx++] = byte;
		bitSize -= CHAR_BIT;
	}
	if (bitSize == 0) return 1;
	byte = data[currentByte];
	while (bitSize > 0) {
		Add(b,byte&0x80);
		byte <<= 1;
		bitSize--;
	}
	return 1;
}

static BitString * StringToBitString(unsigned char * str)
{
	int i=0;
	BitString *result;
	unsigned char *save = str;
	if (str == NULL)		return NULL;
	if (*str == '0' && (str[1] == 'B' || str[1] == 'b'))
		str += 2;
	while (*str) {
		while (*str == ' ' || *str == '\t' || *str == ',')
			str++;
		if (*str == 0)
			break;
		if (*str == '1' || *str == '0') {
			i++;
			str++;
		}
		else break;
	}
	if (i == 0)
		return NULL;
	result = Create(i);
	result->count = i;
	i=0;
	str--;
	while (str >= save) {
		while (*str == ' ' || *str == '\t' || *str == ',')
			str--;
		if (*str == 0)
			break;
		if (*str == '1' || *str == '0') {
			if (*str == '1')
				result->contents[i>>3] |= (1 << (i&7));
			str--;
			i++;
		}
		else break;
	}
	return result;
}


static size_t Print(BitString *b,size_t bufsiz,unsigned char *out)
{
	size_t result=0,j=b->count;
	unsigned char *top = out+bufsiz-1;
	size_t i;
	
	for (i=b->count-1; j>0; i--) {
		if (out && out >= top)
			break;
		if (i!= b->count-1 && 0 == (j&3)) {
			if (out) *out++ = ' ';
			result++;
		}
		if (out && out >= top)
			break;
		if (i!= b->count-1 && 0 == (j&7)) {
			if (out) *out++ = ' ';
			result++;
		}
		if (out && out >= top)
			break;
		if (out) {
			if (b->contents[i>>3] & BitIndexMask[i&7])
				*out = '1';
			else
				*out = '0';
			out++;
		}
		result++;
		if (out && out >= top)
			break;
		j--;
	}
	if (out) *out = 0;
	result++;
	return result;
}

static int Append(BitString *b1,BitString *b2)
{
        size_t bytesize,i,idx;
        int r;

		if (b1 == NULL || b2 == NULL) {
			return NullPtrError("Append");
		}
        r = SetCapacity(b1,b1->count+b2->count);
        if (r < 0)
                return r;
        bytesize = bytesizeFromBitLen(b1->count)+ bytesizeFromBitLen(b2->count);
        for (i=b1->count;i<bytesize;i++) {
                idx = i-b1->count;
                if ((b2->contents[idx >>3] >>(idx&7))&1) {
                        b1->contents[i >>3] |= 1 << (i&7);
                }
                b1->count++;
        }
        return 1;
}


static BitString *Reverse(BitString *b)
{
	BitString *result;
	size_t i,j;

	if (b == NULL)		return NULL;
	result = Create(b->count);
	result->count = b->count;
	i = 0;
	j = b->count-1;
	while (i < result->count) {
		if (b->contents[j >> 3] & (1 << (j&7))) {
			result->contents[i >> 3] |= (1 << (i&7));
		}
		i++;
		j--;
	}
	return result;
}

static size_t Sizeof(BitString *b)
{
	size_t result = sizeof(BitString);
	if (b )		
		result += b->capacity;
	return result;
}


static unsigned GetFlags(BitString *b)
{
	return b->Flags;
}

static unsigned SetFlags(BitString *b,unsigned newval)
{
	int result = b->Flags;
	b->Flags = newval;
	b->timestamp++;
	return result;
}

static size_t expandBitstring(BitString *b)
{
	BIT_TYPE *oldContents = b->contents;
	b->contents = realloc(b->contents,b->capacity+CHUNK_SIZE);
	if (b->contents == NULL) {
		b->contents = oldContents;
		return 0;
	}
	b->capacity += CHUNK_SIZE;
	return b->capacity;
}

static int Add(BitString *b,int newval)
{
	size_t bytepos,bitpos;

	/* Test if the new size + 1 will fit */
	bytepos = (1+b->count) >> 3;
	if (bytepos >= b->capacity) {
		if (!expandBitstring(b))	
			return CONTAINER_ERROR_NOMEMORY;
	}
	bytepos = b->count>>3;
	bitpos = b->count&(CHAR_BIT-1);
	if (newval)		
		b->contents[bytepos] |= 1 << bitpos;
	else
		b->contents[bytepos] &= ~(1 << bitpos);
	b->count++;
	return 1;
}

static int ReplaceAt(BitString *b,size_t idx,int newval)
{
	size_t bytepos,bitpos;

	if (idx >= b->count) {
		iError.RaiseError("ReplaceAt",CONTAINER_ERROR_INDEX,b,idx);
		return 0;

	}
	bytepos = (idx/CHAR_BIT);
	bitpos = idx&(CHAR_BIT-1);
	if (newval)		b->contents[bytepos] |= BitIndexMask[bitpos];
	else
		b->contents[bytepos] &= ~BitIndexMask[bitpos];
	return 1;
}

static int PopBack(BitString *b){
	size_t bytepos,bitpos,idx;
	int result;

	if (b->count == 0) {
		iError.RaiseError("PopBack",CONTAINER_ERROR_INDEX,b,1);
		return CONTAINER_ERROR_INDEX;
	}
	idx = b->count-1;
	bytepos = (idx >> 3);
	bitpos = idx &(CHAR_BIT-1);
	if (b->contents[bytepos]&(1 << bitpos))		
		result = 1;
	else result = 0;
	b->count--;
	return result;
}

static int Apply(BitString *b,int (*Applyfn)(int,void *),void *arg)
{
	size_t i,bytepos,bitpos;

	if (b == NULL)
		return NullPtrError("Apply");
	for (i=0; i<b->count;i++) {
		bytepos = (i/CHAR_BIT);
		bitpos = i & (CHAR_BIT-1);
		Applyfn(b->contents[bytepos]&(1 << bitpos),arg);
	}
	return 1;
}

/*------------------------------------------------------------------------
 Procedure:     bitBitstr ID:1
 Purpose:       Finds the bit position of the given bit-pattern in
                the given text. The algorithm is as follows:
                (1) It builds a table of CHAR_BIT lines, each line
                holding a one bit right shifted pattern.
                (2) For each byte in the text it will compare the
                byte with each byte in the shifted pattern table. A
                match is found if all bytes are equal.
                (3) The first byte and the last byte must be masked
                because they contain some bit positions with unused
                bits.
 Input:         The bit-string to be searched and its length, and
                the bit-pattern and its length. Both lengths in bits
 Output:        Zero if the pattern isn't found, or 1+bit-index of
                the starting bit of the pattern if the pattern is
                found.
 Errors:        If there is no memory it will return "pattern not
                found" (zero).
------------------------------------------------------------------------*/
static size_t bitBitstr(BitString *Text,BitString *Pat)
{
	unsigned char *text = Text->contents;
	size_t textSize = Text->count;
	size_t patSize = Pat->count;
	size_t patByteLength = BYTES_FROM_BITS(patSize);
	size_t i,j,k,stop;
	unsigned char *ShiftTable,*ptable,*pText;

	if (patSize > textSize) /* eliminate wrong arguments */
		return 0;
	/* Allocate the shift table. If no memory abort with zero. */
	ShiftTable = Text->Allocator->malloc(patByteLength*CHAR_BIT);
	if (ShiftTable == NULL) {
		return 0;
	}
	/* The first line of the shift table is unshifted. Just copy it */
	memcpy(ShiftTable,text,patByteLength);
	ptable = ShiftTable + patByteLength; /* Advance poinbter to next line */
	for (i=1; i<CHAR_BIT; i++) {
		/* Copy the last line into the new line */
		memcpy(ptable,ptable-patByteLength,patByteLength);
		/* Now shift it by one bit */
		k=patByteLength;
		for (j = 0; k--; ptable++) {
			j = (j << CHAR_BIT) | *ptable;
			*ptable = (unsigned char)(j >> 1);
		}
	}

	/* We should stop when the number of bytes left is less than the pattern length */
	stop = BYTES_FROM_BITS(textSize) - patByteLength;

	/* For each byte in the input text */
	for (k=0; k <= stop; k++) {
		/* For each line in the shift table */
		for (i=0; i<CHAR_BIT;i++) {
			ptable = ShiftTable+i*patByteLength; /* will point to the table line */
			pText = text+k; /* Will point at the current start character in the text */
			/* The first character needs a mask since there could be
			unused bits */
			if ((*pText++ & mask[i]) != *ptable++)				
				break;
			/* Search each byte in the input text until last byte minus 1 */
			for (j=1; j<patByteLength-1; j++) {
				if (*pText++ != *ptable++)					
					break;
			}
			if (j == patByteLength-1) {
				/* Test the last byte with the complemented mask */
				if ((*pText & maskI[patSize&(CHAR_BIT-1)]) == *ptable) {
					free(ShiftTable);
					return 1+(k*CHAR_BIT)+i;
				}
			}
		}
	}
	/* There was no match */
	Text->Allocator->free(ShiftTable);
	return 0;
}

static int Contains(BitString *txt,BitString *pattern,void *Args)
{
	return bitBitstr(txt,pattern) ? 1 : 0;
}

static int IndexOf(BitString *b,int bit,void *ExtraArgs,size_t *result)
{
	size_t bitpos;
	size_t bytepos,i;

	if (b == NULL)
		return NullPtrError("IndexOf");
	for (i=0; i<b->count;i++) {
		bytepos = i/CHAR_BIT;
		bitpos = i&(CHAR_BIT-1);
		if (bit) {
			if (b->contents[bytepos]&(1 << bitpos)) {
				*result = i;
				return 1;
			}
		}
		else if ((b->contents[bytepos]&(1 << bitpos))==0) {
			*result = i;
			return 1;
		}
	}
	return CONTAINER_ERROR_NOTFOUND;
}
#if 0
/*------------------------------------------------------------------------
Procedure:     ShiftRightByOne ID:1
Purpose:       Shift the given bit pattern 1 bit to the right. This
               was proposed by Peter Nilsson in the comp.lang.c
			   group. It uses a barrel shifter.
Input:         A pattern, and its count in bits
Output:        The shifted pattern. This is done destructively.
Errors:        None
------------------------------------------------------------------------*/
static int ShiftRightByOne(unsigned char *p, size_t z,unsigned carry)
{

	while ( z-- ) {
		carry = (carry << CHAR_BIT) | *p;
		*p++ = carry >> 1;
	}

	return carry & 1;
}
#endif

static int ShiftLeftByOne(unsigned char *p, size_t z,unsigned carry)
{

	while ( z-- ) {
		*p = (unsigned char)((*p << 1)|carry);
		p++;
		carry = ((*p)>>(CHAR_BIT-1))?1:0;
	}

	return carry & 1;
}


static size_t InsertAt(BitString *b,size_t idx,int value)
{
	size_t bitpos,newval;
	unsigned carry;
	int oldval;
	size_t bytepos,count,bytesToShift,bytesUsed;

	if (b == NULL)
		return NullPtrError("InsertAt");
	count = b->count+1;
	bytesUsed = BYTES_FROM_BITS(count);
	if (bytesUsed >= b->capacity) {
		if (!expandBitstring(b))
			return 0;
	}
	bytepos = idx/CHAR_BIT;
	bitpos = idx & (CHAR_BIT-1);
	oldval = b->contents[bytepos];
	newval=0;
	if (value)
		newval |= BitIndexMask[bitpos];
	if (bitpos > 0)
		newval |= (oldval&mask[bitpos-1]);
	if (bitpos < (CHAR_BIT-1))
		newval |= (oldval << 1) & (((unsigned)-1) << (bitpos+1));
	carry = (oldval << (CHAR_BIT-1))?1: 0;
	b->contents[bytepos] = (unsigned char )newval;
	idx += CHAR_BIT-bitpos;
	bytesToShift = bytesUsed - (idx/CHAR_BIT);
	bytepos++;
	ShiftLeftByOne(&b->contents[bytepos],bytesToShift,carry);
	b->count++;
	return b->count;

}

static size_t Insert(BitString *b,int bit){
	return InsertAt(b,0,bit);
}

/*------------------------------------------------------------------------
Procedure:     EraseAt ID:1
Purpose:       Removes the indicated bit from the bitstring, making
               it shorter by 1 bit.
Input:         The bitstring and the index
Output:        Either 1 (success) or zero (failure)
Errors:        No error functions are called. It returns just zero.
               The rationale behind is that you can remove bits in
               a loop until there are none.
------------------------------------------------------------------------*/
static int EraseAt(BitString *bitStr,size_t idx)
{
	size_t bitpos,oldval,newval;
	size_t bytepos,bytesize;
	unsigned tmp;

	if (bitStr == NULL)
		return NullPtrError("EraseAt");

	if (bitStr->count == 0) /* If bit string empty there is nothing to remove */
		return 0;
	if (bitStr->count <= idx) /* if the index is beyond the data return failure */
		idx = bitStr->count-1;
	bytepos = idx/8;
	bitpos = idx & 7;
	oldval = bitStr->contents[bytepos];
	/* The expression 0xff >> (8-bitpos) should
	   yield a mask of 1s in the placces up to
	   to given bit index. For example if we
	   want to erase bit 2, we have 0xff >> 6
	   what yields 3 --> the lower 11 bits
	*/
	newval = oldval &(0xff >>(8-bitpos));
	/* The expression 0xff << bitpos should
	   yield a mask of 1s in the higher positions
	   and zeroes in the lower positions i.e. the
	   indices smaller than the bit we want to erase.
	   We OR the masked higher bits shifted by 1 to the right
	   since we have erased one bit
	 */
	newval |= (oldval>> 1)&(0xff << bitpos);
	if (bitStr->count > 8)
		newval |= (bitStr->contents[bytepos+1]&0x1)?0x80:0;
	bitStr->contents[bytepos] = (unsigned char)newval;
	bytepos++;
	bytesize = bitStr->count >> 3;
	/* Now shift right all the bytes by one, and or
	   the bit being right shifted into the lower byte */
	while (bytepos < bytesize) {
		tmp = bitStr->contents[bytepos+1] &1;
		if (tmp)
			tmp = 0x80;
		bitStr->contents[bytepos] =  (unsigned char)(bitStr->contents[bytepos] >> 1)|tmp;
		bytepos++;
	}
	/* Now right shift the last byte, shifting
	   in a zero */
	bitStr->contents[bytepos] >>= 1;
	bitStr->count--;
	return 1;
}

static int Erase(BitString *bitStr,int val)
{
	size_t idx;
	int i;
	if (bitStr == NULL)
		return NullPtrError("Erase");
	i = IndexOf(bitStr,val,NULL,&idx);
	if (i <0)
		return i;
	EraseAt(bitStr,idx);
	return 1;
}

static int Memset(BitString *b,size_t start,size_t stop,int newval)
{
	BIT_TYPE *contents;
	size_t startbyte,stopbyte;

	if (b == NULL)
		return NullPtrError("Set");
	contents = b->contents;
	if (start >= b->count) {
		iError.RaiseError("iBitstring.Set",CONTAINER_ERROR_INDEX,b,start);
		return CONTAINER_ERROR_INDEX;
	}
	if (stop > b->count)
		stop = b->count;
	startbyte = start >> 3;
	stopbyte = stop >> 3;
	if (newval == 0) {
		if (startbyte == stopbyte) {
			contents[startbyte] &= ((0xff >> (CHAR_BIT - (start&(CHAR_BIT-1)))) |
				    (0xff << ((stop&0x7) + 1)));
		}
		else {
			contents[startbyte] &= 0xff >> (8 - (start&0x7));
			while (++startbyte < stopbyte)
                 contents[startbyte] = 0;
			contents[stopbyte] &= 0xff << ((stop&0x7) + 1);
		}
	}
	else {
        if (startbyte == stopbyte) {
                contents[startbyte] |= ((0xff << (start&0x7)) &
                                    (0xff >> (7 - (stop&0x7))));
        } else {
                contents[startbyte] |= 0xff << ((start)&0x7);
                while (++startbyte < stopbyte)
                        contents[startbyte] = 0xff;
                contents[stopbyte] |= 0xff >> (7 - (stop&0x7));
        }
	}
	return 1;
}

static size_t bitcount(uintmax_t x)
{
	if (64 == sizeof(x)*CHAR_BIT) {
	    x -=  (x>>1) & 0x5555555555555555ULL;                                /* 0-2 in 2 bits */
	    x  = ((x>>2) & 0x3333333333333333ULL) + (x & 0x3333333333333333ULL);  /* 0-4 in 4 bits */
	    x  = ((x>>4) + x) & 0x0f0f0f0f0f0f0f0fULL;                           /* 0-8 in 8 bits */
	    x *= 0x0101010101010101ULL;
	    return   x>>56;
	}
	else { /* Assume that 32 bit unsigned long exists. */
	    x -=  (x>>1) & 0x55555555UL;                        /* 0-2 in 2 bits */
	    x  = ((x>>2) & 0x33333333UL) + (x & 0x33333333UL);  /* 0-4 in 4 bits */
	    x  = ((x>>4) + x) & 0x0f0f0f0fUL;                   /* 0-8 in 8 bits */
	    x *= 0x01010101UL;
	    return  (unsigned)(x>>24);
	}
}

static uintmax_t PopulationCount(BitString *b)
{
	uintmax_t *pumax,x,result=0;
	size_t iterations;
	size_t count;
	unsigned char *p;

	if (b == NULL || b->count == 0)
		return 0;
	/* Treat 32 or 64 bits at once */
	pumax = (uintmax_t *)b->contents;
	iterations = b->count/(sizeof(x)*CHAR_BIT);
	/* Position the pointer p to get the last few bytes
	   that are left once the uintmax_t pointer finishes.
	*/
	p = b->contents+(iterations*sizeof(x));
	while (iterations > 0) {
		x = *pumax++;
		if (x != 0)
			result += bitcount(x);
		iterations--;
	}
	count = (b->count%(sizeof(x)*CHAR_BIT));
	if (count) {
		x=0;
		while (count > CHAR_BIT) {
			x = (x << CHAR_BIT) | *p++;
			count -= CHAR_BIT;
		}
		x = x << CHAR_BIT;
		result += bitcount(x|*p);
	}
	return result;
}

static uintmax_t BitBlockCount(BitString *b)
{
	uintmax_t x=0,result=0;
	unsigned char *pbits;
	size_t iterations;
	size_t count;

	if (b == NULL || b->count == 0)
		return 0;
	/* Treat 32 or 64 bits at once */
	pbits = b->contents;
	iterations = BYTES_FROM_BITS(b->count);
	/* Position the pointer p to get the last few bytes
	   that are left once the uintmax_t pointer finishes.
	*/
	while (iterations > 0) {
		x = *pbits++;
		if (x != 0)
			result += (x & 1) + bitcount( (x^(x>>1)) ) / 2;
		if (iterations > 1 && (x&1) ) {
			if (*pbits >> (CHAR_BIT-1)) {
				result--;
			}
		}
		iterations--;
	}
	count = BITPOS(b->count);
	if (count) {
		x = (*pbits)&mask[count];
		result += (x & 1) + bitcount( (x^(x>>1)) ) / 2;
	}
	return result;
}

/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */

struct BitstringIterator {
	Iterator it;
	BitString *Bits;
	size_t index;
	size_t timestamp;
	int bit;
};

static void *GetNext(Iterator *it)
{
	struct BitstringIterator *bi = (struct BitstringIterator *)it;
	BitString *b = bi->Bits;

	if (bi == NULL) {
		NullPtrError("GetNext");
		return NULL;
	}
	if (bi->timestamp != b->timestamp) {
		return NULL;
	}
	if (bi->index >= b->count)
		return NULL;
	bi->bit = GetElement(b,bi->index++);
	return &bi->bit;
}
static void *GetCurrent(Iterator *it)
{
	struct BitstringIterator *bi = (struct BitstringIterator *)it;
	if (bi == NULL) {
		NullPtrError("GetCurrent");
		return NULL;
	}
	return &bi->bit;
}

static void *GetFirst(Iterator *it)
{
	struct BitstringIterator *bi = (struct BitstringIterator *)it;

	if (bi == NULL) {
		NullPtrError("GetFirst");
		return NULL;
	}
	if (bi->Bits->count == 0)
		return NULL;
	bi->index = 0;
	return GetNext(it);
}

static Iterator *NewIterator(BitString *bitstr)
{
	struct BitstringIterator *result = bitstr->Allocator->malloc(sizeof(struct BitstringIterator));
	if (result == NULL)
		return NULL;
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetNext;
	result->it.GetFirst = GetFirst;
	result->it.GetCurrent = GetCurrent;
	result->Bits = bitstr;
	result->timestamp = bitstr->timestamp;
	return &result->it;
}

static int InitIterator(BitString *bitstr,void *buf)
{
        struct BitstringIterator *result = buf;
        result->it.GetNext = GetNext;
        result->it.GetPrevious = GetNext;
        result->it.GetFirst = GetFirst;
        result->it.GetCurrent = GetCurrent;
        result->Bits = bitstr;
        result->timestamp = bitstr->timestamp;
        return 1;
}


static int deleteIterator(Iterator * it)
{
	struct BitstringIterator *bi = (struct BitstringIterator *)it;
	bi->Bits->Allocator->free(bi);
	return 1;
}
static int Save(const BitString *bitstr,FILE *stream, SaveFunction saveFn,void *arg)
{
	if (stream == NULL || bitstr == NULL) {
		return NullPtrError("Load");
	}
	if (fwrite(bitstr,1,sizeof(BitString),stream) == 0)
		return EOF;
	if (fwrite(bitstr->contents, 1,BYTES_FROM_BITS(bitstr->count), stream) == 0)
		return EOF;
	return 0;
}

static BitString *Load(FILE *stream, ReadFunction readFn,void *arg)
{
	BitString tmp,*r;
	
	if (stream == NULL) {
		NullPtrError("Load");
		return NULL;
	}
	if (fread(&tmp,1,sizeof(BitString),stream) == 0)
		return NULL;
	r = Create(tmp.count);
	if (r == NULL)
		return NULL;
	r->count = tmp.count;
	r->Flags = tmp.Flags;
	if (fread(r->contents,1,BYTES_FROM_BITS(r->count),stream) == 0) {
		iError.RaiseError("Bitstring.Load",CONTAINER_ERROR_FILE_READ);
		return NULL;
	}
	return r;
}

static ErrorFunction *SetErrorFunction(BitString *AL,ErrorFunction fn)
{
	return NULL;
}

static size_t GetElementSize(BitString *B)
{
	return 1;
}
static unsigned char *GetData(BitString *b)
{
	if (b == NULL) {
		NullPtrError("GetBits");
		return NULL;
	}
	return b->contents;
}

static int CopyBits(BitString *b,void *buf)
{
	if (b == NULL) {
		return NullPtrError("CopyBits");
	}
	if (buf == NULL) {
		iError.RaiseError("CopyBits",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	memcpy(buf,b->contents,1+b->count/CHAR_BIT);
	return 1;
}
static BitString *Init(BitString *set, size_t bitlen)
{
	size_t bytesiz;

	memset(set,0,sizeof(BitString));
	bytesiz = 8+bytesizeFromBitLen(bitlen);
	bytesiz = roundup(bytesiz);
	set->contents = CurrentAllocator->malloc(bytesiz);
	if (set->contents == NULL) {
		return NULL;
	}
	memset(set->contents,0,bytesiz);
	set->VTable = &iBitString;
	set->Allocator = CurrentAllocator;
	set->capacity = bytesiz;
	return set;
}

static const ContainerAllocator *GetAllocator(const BitString *b)
{
    if (b == NULL)
        return NULL;
    return b->Allocator;
}

static BitString *CreateWithAllocator(size_t bitlen,const ContainerAllocator *allocator)
{
	BitString *set,*result;

	set = 	CurrentAllocator->malloc(sizeof(BitString));
	if (set == NULL)
		return NULL;
	result = Init(set,bitlen);
	if (result == NULL)
		CurrentAllocator->free(set);
	return result;
}

static BitString    *Create(size_t elementsize)
{
    return CreateWithAllocator(elementsize, CurrentAllocator);
}

BitStringInterface iBitString = {
	GetCount,
	GetFlags,
	SetFlags,
	Clear,
	Contains,
	Erase,
	Finalize,
	Apply,
	Equal,
	Copy,
	SetErrorFunction,
	Sizeof,
	NewIterator,
        InitIterator,
	deleteIterator,
	Save,
	Load,
	GetElementSize,
/* Sequential container part */
	Add,
	GetElement,
	Add, /* Add and push are identical */
	PopBack,
	InsertAt,
	EraseAt,
	ReplaceAt,
	IndexOf,
/* Bitstring specific functions */
	Insert,
	SetElement,
	GetCapacity,
	SetCapacity,
	Or,
	OrAssign,
	And,
	AndAssign,
	Xor,
	XorAssign,
	Not,
	NotAssign,
	PopulationCount,
	BitBlockCount,
	LessEqual,
	Reverse,
	GetRange,
	StringToBitString,
	InitializeWith,
	BitLeftShift,
	BitRightShift,
	Print,
	Append,
	Memset,
	CreateWithAllocator,
	Create,
	Init,
	GetData,
	CopyBits,
	AddRange,
	GetAllocator,
};

