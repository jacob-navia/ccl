/*------------------------------------------------------------------------
Module:        strcollection.c
Author:        jacob
Project:       Containers
State:
Creation Date:
Description:   Implements the string collection container.
Added fixes from Gerome. Oct. 20 2005
------------------------------------------------------------------------*/
#include "containers.h"
#include "ccl_internal.h"

/* Definition of the String Collection type */
struct StringCollection {
    StringCollectionInterface *VTable; /* The table of functions */
    size_t count;                  /* in element size units */
    unsigned int Flags;             /* Read-only or other flags */
    unsigned char **contents;               /* The contents of the collection */
    size_t capacity;                /* in element_size units */
	size_t timestamp;
	ErrorFunction RaiseError;
	StringCompareFn strcompare;
	CompareInfo *StringCompareContext;
    ContainerMemoryManager *Allocator;
};

static const guid StringCollectionGuid = {0x64bea19b, 0x243b, 0x487a,
{0x9a,0xd6,0xcd,0xfe,0xa9,0x37,0x6e,0x88}
};




#ifdef UNIX
#define stricmp strcasecmp
#endif
#ifdef _MSC_VER
#define stricmp _stricmp
#else
extern int stricmp(const char *,const char *);
#endif

#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif

static int NullPtrError(const char *fnName)
{
	return iError.LibraryError("iStringCollection",fnName,CONTAINER_ERROR_BADARG);
}
static int doerrorCall(ErrorFunction err,const char *fnName,int code)
{
	char buf[256];

	sprintf(buf,"iDictionary.%s",fnName);
	err(buf,code);
	return code;
}
static int ReadOnlyError(StringCollection *SC,const char *fnName)
{
	return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_READONLY);
}

static int BadArgError(StringCollection *SC,const char *fnName)
{
	return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_BADARG);
}


static int IndexError(StringCollection *SC,const char *fnName)
{
	return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_INDEX);
}

static int NoMemoryError(StringCollection *SC,const char *fnName)
{
	return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_NOMEMORY);
}

static int Strcmp(const unsigned char *s1,const unsigned char *s2,CompareInfo *ci)
{
	return strcmp((char *)s1,(char *)s2);
}

#if 0
static int Stricmp(unsigned char *s1,unsigned char *s2,CompareInfo *ci)
{
	return stricmp((char *)s1,(char *)s2);
}
#endif

static int CompareStrings(const void *s1,const void *s2)
{
	const char *S1 = *(const char **)s1;
	const char *S2 = *(const char **)s2;
	return strcmp(S1,S2);
}

static int CaseCompareStrings(const void *s1,const void *s2)
{
	const char *S1 = *(const char **)s1;
	const char *S2 = *(const char **)s2;
	return stricmp(S1,S2);
}


/*------------------------------------------------------------------------
Procedure:     DuplicateString ID:1
Purpose:       Functional equivalent of strdup with added argument
               in case of error
Input:         The string to duplicate and the name of the calling function
Output:        The duplicated string
Errors:        Duplication of NULL is allowed and returns NULL. If no more
               memory is available the error function is called.
------------------------------------------------------------------------*/
static unsigned char *DuplicateString(StringCollection *SC,unsigned char *str,const char *fnname)
{
    unsigned char *result;
    if (str == NULL)      return NULL;
    result = SC->Allocator->malloc(1+strlen((char *)str));
    if (result == NULL) {
        NoMemoryError(SC,fnname);
        return NULL;
    }
    strcpy((char *)result,(char *)str);
    return result;
}

#define SC_IGNORECASE   (CONTAINER_READONLY << 1)
#define CHUNKSIZE 20



static size_t GetCount(StringCollection *SC)
{ 
	if (SC == NULL) {
		NullPtrError("GetCount");
		return 0;
	}
	return SC->count;
}

/*------------------------------------------------------------------------
 Procedure:     IsReadOnly ID:1
 Purpose:       Reads the read-only flag
 Input:         The collection
 Output:        the state of the flag
 Errors:        None
------------------------------------------------------------------------*/
static unsigned GetFlags(StringCollection *SC)
{
	if (SC == NULL) {
		NullPtrError("GetFlags");
		return 0;
	}
	return SC->Flags;
}

static ContainerMemoryManager *GetAllocator(StringCollection *AL)
{
	if (AL == NULL) {
		return NULL;
	}
	return AL->Allocator;
}

static int Mismatch(StringCollection *a1,StringCollection *a2,size_t *mismatch)
{
	size_t siz,i;
	CompareInfo ci;
	unsigned char **p1,**p2;

	*mismatch = 0;
	if (a1 == a2)
		return 0;
	if (a1 == NULL || a2 == NULL)
		return 1;
	siz = a1->count;
	if (siz > a2->count)
		siz = a2->count;
	if (siz == 0)
		return 1;
	p1 = a1->contents;
	p2 = a2->contents;
	ci.Container = a1;
	ci.ExtraArgs = a2;
	for (i=0;i<siz;i++) {
		if (a1->strcompare(*p1,*p2,&ci) != 0) {
			*mismatch = i;
			return 1;
		}
		p1++,p2++;
	}
	*mismatch = i;
	return a1->count != a2->count;
}
/*------------------------------------------------------------------------
 Procedure:     SetReadOnly ID:1
 Purpose:       Sets the value of the read only flag
 Input:         The collection to be changed
 Output:        The old value of the flag
 Errors:        None
------------------------------------------------------------------------*/
static unsigned SetFlags(StringCollection *SC,unsigned newval)
{
    int oldval;
	
	if (SC == NULL) {
		NullPtrError("SetFlags");
		return 0;
	}
	oldval = SC->Flags;
    SC->Flags = newval;
    return oldval;
}
static int ResizeTo(StringCollection *SC,size_t newcapacity)
{
    unsigned char **oldcontents;

	if (SC == NULL) {
		return NullPtrError("ResizeTo");
	}
	oldcontents = SC->contents;
	SC->contents = SC->Allocator->malloc(newcapacity*sizeof(char *));
    if (SC->contents == NULL) {
		SC->contents = oldcontents;
		return NoMemoryError(SC,"ResizeTo");
    }
    memset(SC->contents,0,sizeof(char *)*newcapacity);
    memcpy(SC->contents,oldcontents,SC->count*sizeof(char *));
    SC->capacity = newcapacity;
	SC->Allocator->free(oldcontents);
	SC->timestamp++;
    return 1;
}

/*------------------------------------------------------------------------
 Procedure:     Resize ID:1
 Purpose:       Grows a string collection by CHUNKSIZE
 Input:         The collection
 Output:        One if successfull, zero othewise
 Errors:        If no more memory is available, nothing is changed
                and returns zero
------------------------------------------------------------------------*/
static int Resize(StringCollection *SC)
{
    return ResizeTo(SC, SC->capacity + 1+SC->capacity/4);
}

/*------------------------------------------------------------------------
 Procedure:     Add ID:1
 Purpose:       Adds a string to the string collection.
 Input:         The collection and the string to be added to it
 Output:        Number of items in the collection or <= 0 if error
 Errors:
------------------------------------------------------------------------*/
static int Add(StringCollection *SC,unsigned char *newval)
{
	if (SC == NULL) {
		return NullPtrError("Add");
	}

	if (SC->Flags & CONTAINER_READONLY)
        return ReadOnlyError(SC,"Add");
    if (SC->count >= SC->capacity) {
		int r = Resize(SC);
        if (r <= 0)
            return r;
    }

    if (newval) {
        SC->contents[SC->count] = DuplicateString(SC,newval,"Add");
        if (SC->contents[SC->count] == NULL) {
            return 0;
        }
    }
    else
        SC->contents[SC->count] = NULL;
	SC->timestamp++;
    ++SC->count;
	return 1;
}

static int AddRange(StringCollection *SC,size_t n, unsigned char **data)
{
    size_t newcapacity;
	unsigned char **p;

	if (n == 0)
		return 1;
	if (SC == NULL) {
		return NullPtrError("AddRange");
	}
	if (data == NULL) {
		return BadArgError(SC,"AddRange");
	}
    if (SC->Flags & CONTAINER_READONLY) {
        return ReadOnlyError(SC,"AddRange");
	}

	newcapacity = SC->count+n;
	if (newcapacity >= SC->capacity-1) {
		unsigned char **newcontents;
		newcapacity += SC->count/4;
		newcontents = SC->Allocator->realloc(SC->contents,newcapacity*sizeof(void *));
		if (newcontents == NULL) {
			return NoMemoryError(SC,"AddRange");
		}
		SC->capacity = newcapacity;
		SC->contents = newcontents;
	}
	p = SC->contents;
	p += SC->count;
	memcpy(p,data,n*sizeof(void *));
	SC->count += n;
	SC->timestamp++;

    return 1;
}

static int Append(StringCollection *SC1, StringCollection *SC2)
{
	if (SC1 == NULL || SC2 == NULL) {
		return NullPtrError("Append");
	}
	if (SC1->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(SC1,"Append");
	}
	return AddRange(SC1,SC2->count,SC2->contents);
}

static int Clear(StringCollection *SC)
{
	size_t i;

	if (SC == NULL) {
		return NullPtrError("Clear");
	}
    if (SC->Flags & CONTAINER_READONLY)
        return ReadOnlyError(SC,"Clear");
    for (i=0; i<SC->count;i++) {
		SC->Allocator->free(SC->contents[i]);
        SC->contents[i] = NULL;
    }
    SC->count = 0;
	SC->timestamp=0;
	SC->Flags=0;
    return 1;
}


static int Contains(StringCollection *SC,unsigned char *str)
{
    int c;
	size_t i;

	if (SC == NULL) {
		return NullPtrError("Contains");
	}
	if (str == NULL)
		return 0;
    c = *str;
    for (i=0; i<SC->count;i++) {
		if (c == SC->contents[i][0] && !SC->strcompare(SC->contents[i],str,NULL))
			return 1;
    }
    return 0;
}

static unsigned char **CopyTo(StringCollection *SC)
{
	unsigned char **result;
	size_t i;

	if (SC == NULL) {
		NullPtrError("CopyTo");
		return NULL;
	}

	result = SC->Allocator->malloc((1+SC->count)*sizeof(char *));
    if (result == NULL) {
		NoMemoryError(SC,"CopyTo");
        return NULL;
	}
    for (i=0; i<SC->count;i++) {
        result[i] = DuplicateString(SC,SC->contents[i],"CopyTo");
		if (result[i] == NULL) {
			while (i > 0) {
				SC->Allocator->free(result[--i]);
			}
			SC->Allocator->free(result);
			return NULL;
		}
    }
    result[i] = NULL;
    return result;
}

static int IndexOf(StringCollection *SC,unsigned char *str,size_t *result)
{
    size_t i;

	if (SC == NULL) {
		return NullPtrError("IndexOf");
	}
	if (str == NULL)
		return BadArgError(SC,"IndexOf");

    for (i=0; i<SC->count;i++) {
		if (!SC->strcompare(SC->contents[i],str,SC->StringCompareContext)) {
			*result = i;
            return 1;
        }
    }
    return CONTAINER_ERROR_NOTFOUND;
}
static int Finalize(StringCollection *SC)
{
	size_t i;

	if (SC == NULL) {
		return NullPtrError("Finalize");
	}
	if (SC->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(SC,"Finalize");
	}

	for (i=0; i<SC->count;i++) {
        SC->Allocator->free(SC->contents[i]);
    }
    SC->Allocator->free(SC->contents);
    CurrentMemoryManager->free(SC);
    return 1;
}

static unsigned char *GetElement(StringCollection *SC,size_t idx)
{
	if (SC == NULL) {
		NullPtrError("GetElement");
		return NULL;
	}
	if (SC->Flags & CONTAINER_READONLY) {
		ReadOnlyError(SC,"GetElement");
		return NULL;
	}
    if (idx >=SC->count) {
		IndexError(SC,"GetElement");
        return NULL;
	}
    return SC->contents[idx];
}

static StringCollection *IndexIn(StringCollection *SC,Vector *AL)
{
	StringCollection *result = NULL;
	size_t i,top,idx;
	unsigned char *p;
	int r;

	if (SC == NULL || AL == NULL) {
		NullPtrError("IndexIn");
		return NULL;
	}
	if (iVector.GetElementSize(AL) != sizeof(size_t)) {
		SC->RaiseError("iStringCollection.IndexIn",CONTAINER_ERROR_INCOMPATIBLE);
		return NULL;
	}
	top = iVector.Size(AL);
	result = iStringCollection.Create(top);
	for (i=0; i<top;i++) {
		idx = *(size_t *)iVector.GetElement(AL,i);
		p = GetElement(SC,idx);
		if (p == NULL)
			goto err;
		r = Add(result,p);
		if (r < 0) {
err:
			Finalize(result);
			return NULL;
		}
	}
	return result;
}

static int InsertAt(StringCollection *SC,size_t idx,unsigned char *newval)
{
    unsigned char *p;
	if (SC == NULL) {
		return NullPtrError("InsertAt");
	}
	if (newval == NULL) {
		return BadArgError(SC,"InsertAt");
	}
    if (SC->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(SC,"InsertAt");
	}
    if (idx >= SC->count) {
        return IndexError(SC,"InsertAt");
	}
    if (SC->count >= SC->capacity) {
        int r = Resize(SC);
		if (r <= 0)
            return r;
    }
    p = DuplicateString(SC,newval,"InsertAt");
    if (p == NULL) {
        return NoMemoryError(SC,"InsertAt");
    }

    if (idx == 0) {
        if (SC->count > 0)
	        memmove(SC->contents+1,SC->contents,SC->count*sizeof(char *));
        SC->contents[0] = p;
    }
    else if (idx == SC->count) {
        SC->contents[idx] = p;
    }
    else if (idx < SC->count) {
        memmove(SC->contents+idx+1,SC->contents+idx,(SC->count-idx+1)*sizeof(char *));
        SC->contents[idx] = p;
    }
	SC->timestamp++;
    ++SC->count;
	return 1;
}

static int InsertIn(StringCollection *source, size_t idx, StringCollection *newData)
{
	size_t newCount,i,j,siz;
	unsigned char **p,**oldcontents;

	if (source == NULL || newData == NULL) {
		return NullPtrError("InsertIn");
	}
	if (source->Flags & CONTAINER_READONLY)
		return ReadOnlyError(source,"InsertIn");
	if (idx > source->count) {
		return IndexError(source,"InsertIn");
	}
	newCount = source->count + newData->count;
	if (newData->count == 0)
		return 1;
	if (newCount == 0)
		return 1;
	if (newCount >= (source->capacity-1)) {
		int r = ResizeTo(source,1+newCount+newCount/4);
		if (r <= 0)
			return r;
	}
	p = source->contents;
	siz = source->capacity*sizeof(char *);
	oldcontents = source->Allocator->malloc(siz);
	if (oldcontents == NULL) {
		return NoMemoryError(source,"InsertIn");
	}
	memset(oldcontents,0,siz);
	memcpy(oldcontents,p,sizeof(char *)*source->count);
	if (idx < source->count) {
		memmove(p+(idx+newData->count),
				p+idx,
				(source->count-idx)*sizeof(char *));
	}
	for (i=idx,j=0; i<idx+newData->count;i++,j++) {
		source->contents[i] = DuplicateString(newData,newData->contents[j],"InsertIn");
		if (source->contents[i] == NULL) {
			source->Allocator->free(source->contents);
			source->contents = oldcontents;
			return NoMemoryError(source,"InsertIn");
		}
	}
	source->Allocator->free(oldcontents);
	source->timestamp++;
	source->count = newCount;
	return 1;
}

static int Insert(StringCollection *SC,unsigned char *newval)
{
    return InsertAt(SC,0,newval);
}

static int RemoveAt(StringCollection *SC,size_t idx)
{
	if (SC == NULL) {
		return NullPtrError("RemoveAt");
	}
    if (idx >= SC->count )
		return IndexError(SC,"RemoveAt");
	if (SC->Flags & CONTAINER_READONLY)
        return ReadOnlyError(SC,"RemoveAt");
	/* Test for remove of an empty collection */
    if (SC->count == 0)
        return 0;
    SC->Allocator->free(SC->contents[idx]);
    if (idx < (SC->count-1)) {
        memmove(SC->contents+idx,SC->contents+idx+1,(SC->count-idx)*sizeof(char *));
    }
    SC->contents[SC->count-1]=NULL;
	SC->timestamp++;
    --SC->count;
	return 1;
}

static int Erase(StringCollection *SC,unsigned char *str)
{
	size_t i;
	if (SC == NULL) {
		return NullPtrError("Erase");
	}
	if (str == NULL) {
		return BadArgError(SC,"Erase");
	}
	if (SC->Flags & CONTAINER_READONLY)
        return ReadOnlyError(SC,"Erase");
	for (i=0; i<SC->count;i++) {
		if (!SC->strcompare(SC->contents[i],str,SC->StringCompareContext)) {
            break;
        }
    }
	if (i == SC->count)
		return CONTAINER_ERROR_NOTFOUND;
    SC->Allocator->free(SC->contents[i]);
    if (i < (SC->count-1)) {
        memmove(SC->contents+i,SC->contents+i+1,(SC->count-i)*sizeof(char *));
    }
    --SC->count;
    SC->contents[SC->count]=NULL;
	SC->timestamp++;

    return 1;
}

static int PushBack(StringCollection *SC,unsigned char *str)
{
    unsigned char *r;

	if (SC == NULL)
		return NullPtrError("PushBack");
    if (SC->Flags&CONTAINER_READONLY) {
		return ReadOnlyError(SC,"PushBack");
	}
    if (SC->count >= SC->capacity-1) {
        int res = Resize(SC);
		if (res <= 0)
            return res;
    }
    r = DuplicateString(SC,str,"Push");
    if (r == NULL)
        return 0;
    SC->contents[SC->count++] = r;
	SC->timestamp++;
	return 1;
}

static int PushFront(StringCollection *SC,unsigned char *str)
{
    unsigned char *r;

	if (SC == NULL)
		return NullPtrError("PushFront");
	if (str == NULL)
		return BadArgError(SC,"PushFront");
    if (SC->Flags&CONTAINER_READONLY)
        return 0;
    if (SC->count >= SC->capacity-1) {
        int res = Resize(SC);
		if (res <= 0)
            return res;
    }
    r = DuplicateString(SC,str,"PushFront");
    if (r == NULL)
        return NoMemoryError(SC,"PushFront");
	memmove(SC->contents+1,SC->contents,SC->count*sizeof(void *));
    SC->contents[0] = r;
	SC->timestamp++;
	SC->count++;
	return 1;
}

static size_t PopBack(StringCollection *SC,unsigned char *buffer,size_t buflen)
{
    unsigned char *result;
	size_t len,tocopy;

	if (SC == NULL) {
		NullPtrError("PopBack");
		return 0;
	}
    if (SC->Flags&CONTAINER_READONLY)
		return 0;
	if (SC->count == 0)
        return 0;
	len = 1+strlen((char *)SC->contents[SC->count-1]);
	if (buffer == NULL)
		return len;
    SC->count--;
    result = SC->contents[SC->count];
    SC->contents[SC->count] = NULL;
	SC->timestamp++;
	tocopy = len;
	if (buflen < tocopy)
		tocopy = buflen-1;
	memcpy(buffer,result,tocopy);
	buffer[tocopy-1] = 0;
	SC->Allocator->free(result);
    return len;
}

static size_t PopFront(StringCollection *SC,unsigned char *buffer,size_t buflen)
{
    unsigned char *result;
	size_t len,tocopy;

	if (SC == NULL)
		return 0;
    if ((SC->Flags&CONTAINER_READONLY) || SC->count == 0)
        return 0;
	len = 1+strlen((char *)SC->contents[0]);
	if (buffer == NULL)
		return len;
    SC->count--;
    result = SC->contents[0];
	if (SC->count) {
		memcpy(SC->contents,SC->contents+1,SC->count*sizeof(void *));
	}
	SC->timestamp++;
	tocopy = len;
	if (buflen < tocopy)
		tocopy = buflen-1;
	memcpy(buffer,result,tocopy);
	buffer[tocopy-1] = 0;
	SC->Allocator->free(result);
    return len;
}


static size_t GetCapacity(StringCollection *SC)
{
	if (SC == NULL) {
		NullPtrError("SetCapacity");
		return 0;
	}
    return SC->capacity;
}

static int SetCapacity(StringCollection *SC,size_t newCapacity)
{
	unsigned char **newContents;
	if (SC == NULL) {
		return NullPtrError("SetCapacity");
	}
	if (SC->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(SC,"SetCapacity");
	}
	newContents = SC->Allocator->malloc(newCapacity*sizeof(void *));
	if (newContents == NULL) {
		return NoMemoryError(SC,"SetCapacity");
	}
	memset(SC->contents,0,sizeof(void *)*newCapacity);
	SC->capacity = newCapacity;
	if (newCapacity > SC->count)
		newCapacity = SC->count;
	else if (newCapacity < SC->count)
		SC->count = newCapacity;
	if (newCapacity > 0) {
		memcpy(newContents,SC->contents,newCapacity*sizeof(void *));
	}
	SC->Allocator->free(SC->contents);
	SC->contents = newContents;
	SC->timestamp++;
    return 1;
}

static int Apply(StringCollection *SC,int (*Applyfn)(unsigned char *,void *),void *arg)
{
    size_t i;

	if (SC == NULL) {
		return NullPtrError("Apply");
	}
	if (Applyfn == NULL) {
		return BadArgError(SC,"Apply");
	}
    for (i=0; i<SC->count;i++) {
        Applyfn(SC->contents[i],arg);
    }
	return 1;
}

#if 0
static int *Map(StringCollection *SC,int (*Applyfn)(unsigned char *))
{
	int *result = SC->Allocator->malloc(SC->count*sizeof(int));
	size_t i;

    if (result == NULL)
      return NULL;
    for (i=0; i<SC->count;i++) {
        result[i] = Applyfn(SC->contents[i]);
    }
    return result;
}
#endif
/* gedd123@free.fr (gerome) proposed calling DuplicateString. Good
suggestion */
static int ReplaceAt(StringCollection *SC,size_t idx,unsigned char *newval)
{
    unsigned char *r;

	if (SC == NULL) {
		return NullPtrError("ReplaceAt");
	}
    if (SC->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(SC,"ReplaceAt");
	}
    if (idx >= SC->count) {
		return IndexError(SC,"ReplaceAt");
	}
    SC->Allocator->free(SC->contents[idx]);
    r = DuplicateString(SC,newval,(char *)"ReplaceAt");
    if (r == NULL) {
		return NoMemoryError(SC,"ReplaceAt");
	}
    SC->contents[idx] = r;
	SC->timestamp++;
    return 1;
}

static int Equal(StringCollection *SC1,StringCollection *SC2)
{
    size_t i;
	CompareInfo *ci;

	if (SC1 == NULL && SC2 == NULL)
		return 1;
	if (SC1 == NULL || SC2 == NULL)
		return 0;
	if (SC1->count != SC2->count)
		return 0;
	if (SC1->strcompare != SC2->strcompare)
		return 0;
	if (SC1->StringCompareContext != SC2->StringCompareContext &&
		SC1->StringCompareContext != NULL &&
		SC2->StringCompareContext != NULL)
		return 0;
	if (SC1->StringCompareContext != SC2->StringCompareContext) {
		ci = SC1->StringCompareContext ? SC1->StringCompareContext :
			SC2->StringCompareContext;
	}
	else ci = SC1->StringCompareContext;
	for (i=0; i<SC1->count;i++) {
		if (SC1->strcompare(SC1->contents[i],SC2->contents[i],ci))
			return 0;
	}
    return 1;
}

static StringCollection *Copy(StringCollection *SC)
{
    size_t i;
    StringCollection *result;

	if (SC == NULL) {
		NullPtrError("Copy");
		return NULL;
	}
	result = iStringCollection.Create(SC->count);
	if (result) {
		result->VTable = SC->VTable;
		result->strcompare = SC->strcompare;
		for (i=0; i<SC->count;i++) {
			if (SC->VTable->Add(result,SC->contents[i]) <= 0) {
				Finalize(result);
				return NULL;
			}
		}
		result->Flags = SC->Flags;
    }
    return result;
}


static ErrorFunction SetErrorFunction(StringCollection *SC,ErrorFunction fn)
{
	ErrorFunction old;
	if (SC == NULL) {
		NullPtrError("SetErrorFunction");
		return NULL;
	}
	old = SC->RaiseError;
	if (fn)
		SC->RaiseError = (fn);
	return old;
}

static bool Sort(StringCollection *SC)
{
	if (SC == NULL) {
		return NullPtrError("Sort");
	}
	if (SC->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(SC,"Sort");
	}
	if (SC->Flags & SC_IGNORECASE)
		qsort(SC->contents,SC->count,sizeof(char *),CaseCompareStrings);
	else
		qsort(SC->contents,SC->count,sizeof(char *),CompareStrings);
	SC->timestamp++;
	return 1;
}

static size_t Sizeof(StringCollection *SC)
{
	size_t result= sizeof(StringCollection);
	size_t i;

	if (SC == NULL) {
		return sizeof(StringCollection);
	}
	for (i=0; i<SC->count;i++) {
		result += strlen((char *)SC->contents[i]) + 1 + sizeof(char *);
	}
	result += (SC->capacity - SC->count) * sizeof(char *);
	return result;
}

/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */

struct StringCollectionIterator {
	Iterator it;
	StringCollection *SC;
	size_t index;
	size_t timestamp;
	unsigned long Flags;
	unsigned char *current;
};

static void *GetNext(Iterator *it)
{
	struct StringCollectionIterator *sci = (struct StringCollectionIterator *)it;
	StringCollection *SC;

	if (sci == NULL) {
		NullPtrError("GetNext");
		return NULL;
	}
	SC = sci->SC;
	if (sci->timestamp != SC->timestamp) {
		SC->RaiseError("GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	if (sci->index >= SC->count-1)
		return NULL;
    sci->index++;
	sci->current = sci->SC->contents[sci->index];
	return sci->current;
}

static void *GetPrevious(Iterator *it)
{
	struct StringCollectionIterator *ali = (struct StringCollectionIterator *)it;
	StringCollection *SC;

	if (ali == NULL) {
		NullPtrError("GetPrevious");
		return NULL;
	}
	SC = ali->SC;
	if (ali->index >= SC->count || ali->index == 0)
		return NULL;
	if (ali->timestamp != SC->timestamp) {
		SC->RaiseError("GetPrevious",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	ali->index--;
	ali->current = ali->SC->contents[ali->index];
	return ali->current;
}

static void *GetFirst(Iterator *it)
{
	struct StringCollectionIterator *ali = (struct StringCollectionIterator *)it;

	if (ali == NULL) {
		NullPtrError("GetFirst");
		return NULL;
	}
	if (ali->SC->count == 0)
		return NULL;
	ali->index = 0;
	ali->current = ali->SC->contents[0];
	return ali->current;
}

static void *GetCurrent(Iterator *it)
{
	struct StringCollectionIterator *ali = (struct StringCollectionIterator *)it;
	
	if (ali == NULL) {
		NullPtrError("GetCurrent");
		return NULL;
	}
	return ali->current;
}

static Iterator *newIterator(StringCollection *SC)
{
	struct StringCollectionIterator *result;

	if (SC == NULL) {
		NullPtrError("newIterator");
		return NULL;
	}
	result  = SC->Allocator->malloc(sizeof(struct StringCollectionIterator));
	if (result == NULL) {
		NoMemoryError(SC,"newIterator");
		return NULL;
	}
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetPrevious;
	result->it.GetFirst = GetFirst;
	result->it.GetCurrent = GetCurrent;
	result->SC = SC;
	result->timestamp = SC->timestamp;
	result->current = NULL;
	return &result->it;
}

static int deleteIterator(Iterator *it)
{
	struct StringCollectionIterator *sci = (struct StringCollectionIterator *)it;
	StringCollection *SC;

	if (sci == NULL) {
		return NullPtrError("deleteIterator");
	}
	SC = sci->SC;
	SC->Allocator->free(it);
	return 1;
}

static int SaveHeader(StringCollection *SC,FILE *stream)
{
	return (int)fwrite(SC,1,sizeof(StringCollection),stream);
}

static size_t DefaultSaveFunction(const void *element,void *arg, FILE *Outfile)
{
	const unsigned char *str = (const unsigned char *)element;
	size_t len = strlen((const char *)str);

	if (encode_ule128(Outfile, len) <= 0)
		return (size_t)EOF;
	return fwrite(str,1,len+1,Outfile);
}

static size_t DefaultLoadFunction(void *element,void *arg, FILE *Infile)
{
	size_t len = *(size_t *)arg;

	return fread(element,1,len,Infile);
}

static int Save(StringCollection *SC,FILE *stream, SaveFunction saveFn,void *arg)
{
	size_t i;

	if (SC == NULL) {
		return NullPtrError("Save");
	}
	if (stream == NULL) {
		return BadArgError(SC,"Save");
	}
	if (saveFn == NULL)
		saveFn = DefaultSaveFunction;
	if (fwrite(&StringCollectionGuid,sizeof(guid),1,stream) <= 0)
		return EOF;
	if (SaveHeader(SC,stream) <= 0)
		return EOF;
	for (i=0; i< SC->count; i++) {
		if (saveFn(SC->contents[i],arg,stream) <= 0)
			return EOF;
	}
	return 1;
}

static StringCollection *Load(FILE *stream, ReadFunction readFn,void *arg)
{
	size_t i,len;
	StringCollection *result,SC;
	guid Guid;

	if (stream == NULL) {
		NullPtrError("Load");
		return NULL;
	}
	if (readFn == NULL) {
		readFn = DefaultLoadFunction;
		arg = &len;
	}
	if (fread(&Guid,sizeof(guid),1,stream) <= 0) {
		iError.RaiseError("iStringCollection.Load",CONTAINER_ERROR_FILE_READ);
		return NULL;
	}
	if (memcmp(&Guid,&StringCollectionGuid,sizeof(guid))) {
		iError.RaiseError("iStringCollection.Load",CONTAINER_ERROR_WRONGFILE);
		return NULL;
	}
	if (fread(&SC,1,sizeof(StringCollection),stream) <= 0) {
		iError.RaiseError("iStringCollection.Load",CONTAINER_ERROR_FILE_READ);
		return NULL;
	}
	result = iStringCollection.Create(SC.count);
	if (result == 0) {
		return NULL;
	}
	result->Flags = SC.Flags;
	for (i=0; i< SC.count; i++) {
		if (decode_ule128(stream, &len) <= 0) {
			goto err;
		}
		len++;
		result->contents[i] = result->Allocator->malloc(len);
		if (result->contents[i] == NULL) {
			NoMemoryError(result,"Load");
			Finalize(result);
			return NULL;
		}
		if (readFn(result->contents[i],arg,stream) <= 0) {
		err:
			iError.RaiseError("StringCollection.Load",CONTAINER_ERROR_FILE_READ);
			break;
		}
		result->count++;
	}
	return result;
}
static Vector *CastToArray(StringCollection *SC)
{
	Vector *AL = iVector.Create(sizeof(void *),SC->count);
	size_t i;

	for (i=0; i<SC->count;i++) {
		iVector.Add(AL,SC->contents[i]);
	}
	return AL;
}

/* Bug fixes proposed by oetelaar.automatisering */
static StringCollection *CreateFromFile(unsigned char *fileName)
{
	StringCollection *result;
	unsigned char *line=NULL;
	int llen=0,r;
	ContainerMemoryManager *mm = CurrentMemoryManager;
	FILE *f;

	if (fileName == NULL) {
		NullPtrError("CreateFromFile");
		return NULL;
	}
	f = fopen((char *)fileName,"r");
	if (f == NULL) {
		iError.RaiseError("iStringCollection.CreateFromFile",CONTAINER_ERROR_NOENT);
		return NULL;
	}
	result = iStringCollection.Create(10);
	if (result == NULL) {
		fclose(f); /* Was missing! */
		return NULL;
	}
	r = GetLine(&line,&llen,f,mm);
	while (r >= 0) {
		if (iStringCollection.Add(result,line) <= 0) {
			Finalize(result);
			free(line); /* was missing! */
			fclose(f);
			return NULL;
		}
		r = GetLine(&line,&llen,f,mm);
	}
	if (r != EOF) {
		Finalize(result);
		free(line);
		fclose(f);
		return NULL;
	}
	free(line);
	fclose(f);
	return result;
}

static StringCollection *GetRange(StringCollection *SC, size_t start,size_t end)
{
	StringCollection *result;
	size_t idx=0;
	
	if (SC == NULL) {
		NullPtrError("GetRange");
		return NULL;
	}
	result = SC->VTable->Create(SC->count);
	result->VTable = SC->VTable;
	if (SC->count == 0)
		return result;
	if (end >= SC->count)
		end = SC->count;
	if (start > end)
		return result;
	while (start < end) {
		result->contents[idx] = DuplicateString(SC,SC->contents[start],"GetRange");
		if (result->contents[idx] == NULL) {
			while (idx > 0) {
				if (idx > 0)
					idx--;
				SC->Allocator->free(result->contents[idx]);
			}
			SC->Allocator->free(SC->contents);
			SC->Allocator->free(result);
			NoMemoryError(SC,"GetRange");
			return NULL;
		}
		start++;
	}
	result->Flags = SC->Flags;
	return result;
}


static int WriteToFile(StringCollection *SC,unsigned char *fileName)
{
	FILE *f;
	size_t i;

	if (SC == NULL || fileName == NULL) {
		return NullPtrError("WriteToFile");
	}
	f = fopen((char *)fileName,"w");
	if (f == NULL) {
		SC->RaiseError("iStringCollection.WriteToFile",CONTAINER_ERROR_FILEOPEN);
		return CONTAINER_ERROR_FILEOPEN;
	}
	for (i=0; i<SC->count;i++) {
		if (fwrite(SC->contents[i],1,strlen((char *)SC->contents[i]),f) <= 0) {
			iError.RaiseError("iStringCollection.WriteToFile",CONTAINER_ERROR_FILE_WRITE);
			fclose(f);
			return CONTAINER_ERROR_FILE_WRITE;
		}
	}
	fclose(f);
	return SC->count ? 1:0;
}

static size_t FindFirstText(StringCollection *SC,unsigned char *text)
{
	size_t i;

	if (SC == NULL || text == NULL)
		return 0;
	for (i=0; i<SC->count;i++) {
		if (strstr((char *)SC->contents[i],(char *)text)) {
			return i+1;
		}
	}
	return 0;
}

static StringCompareFn SetCompareFunction(StringCollection *SC,StringCompareFn fn)
{
	StringCompareFn oldFn;
	if (SC == NULL) {
		NullPtrError("SetCompareFunction");
		return 0;
	}
	oldFn = SC->strcompare;
	if (fn)
		SC->strcompare = fn;
	return oldFn;
}

static size_t FindNextText(StringCollection *SC, unsigned char *text, size_t start)
{
	size_t i;

	if (SC == NULL || text == NULL)
		return 0;
	for (i=start; i<SC->count;i++) {
		if (strstr((char *)SC->contents[i],(char *)text)) {
			return i+1;
		}
	}
	return 0;
}

static StringCollection *FindText(StringCollection *SC,unsigned char *text)
{
	StringCollection *result = NULL;
	size_t i;

	for (i=0; i<SC->count;i++) {
		if (strstr((char *)SC->contents[i],(char *)text)) {
			if (result == NULL) {
				result = iStringCollection.Create(sizeof(size_t));
				if (result == NULL)
					return NULL;
			}
			if (iStringCollection.Add(result,SC->contents[i]) <= 0)
				break;
		}
	}
	return result;
}

static Vector *FindTextIndex(StringCollection *SC,unsigned char *text)
{
	Vector *result = NULL;
	size_t i;

	for (i=0; i<SC->count;i++) {
		if (strstr((char *)SC->contents[i],(char *)text)) {
			if (result == NULL) {
				result = iVector.Create(sizeof(size_t),10);
				if (result == NULL)
					return NULL;
			}
			if (iVector.Add(result,SC->contents[i]) <= 0)
				break;
		}
	}
	return result;
}

static Vector *FindTextPositions(StringCollection *SC,unsigned char *text)
{
	Vector *result = NULL;
	unsigned char *p;
	size_t i,idx;

	for (i=0; i<SC->count;i++) {
		if (NULL != (p=(unsigned char *)strstr((char *)SC->contents[i],(char *)text))) {
			if (result == NULL) {
				result = iVector.Create(sizeof(size_t),10);
				if (result == NULL)
					return NULL;
			}
			idx = p - SC->contents[i];
			if (iVector.Add(result,&i) <= 0)
				break;
			if (iVector.Add(result,&idx) <=0)
				break;
		}
	}
	return result;
}
/*------------------------------------------------------------------------
 Procedure:     Create ID:1
 Purpose:       Creates a new string collection
 Input:         The initial start size of the collection
 Output:        A pointer to the new collection or NULL
 Errors:        If no more memory is available it returns NULL
 after calling the error function.
 ------------------------------------------------------------------------*/

static StringCollection *InitWithAllocator(StringCollection *result,size_t startsize,ContainerMemoryManager *allocator)
{
    memset(result,0,sizeof(StringCollection));
    result->VTable = &iStringCollection;
    if (startsize > 0) {
        result->contents = CurrentMemoryManager->malloc(startsize*sizeof(char *));
        if (result->contents == NULL) {
            iError.RaiseError("iStringCollection.Create",CONTAINER_ERROR_NOMEMORY);
            return NULL;
        }
        else {
            memset(result->contents,0,sizeof(char *)*startsize);
            result->capacity = startsize;
        }
    }
	result->RaiseError = iError.RaiseError;
	result->strcompare = Strcmp;
	result->Allocator = allocator;
    return result;

}
static StringCollection  *CreateWithAllocator(size_t startsize,ContainerMemoryManager *allocator)
{
    StringCollection *result,*r1;

    r1 = allocator->malloc(sizeof(*result));
    if (r1 == NULL) {
        iError.RaiseError("iStringCollection.Create",CONTAINER_ERROR_NOMEMORY);
        return NULL;
    }
	result = InitWithAllocator(r1,startsize,allocator);
	if (r1 == NULL) {
		allocator->free(result);
	}
    return result;
}

static StringCollection *Init(StringCollection *result,size_t startsize)
{
	return InitWithAllocator(result,startsize,CurrentMemoryManager);
}

static StringCollection *Create(size_t startsize)
{
	return CreateWithAllocator(startsize,CurrentMemoryManager);
}

static size_t GetElementSize(StringCollection *sc)
{
	return sizeof(void *);
}

/* Proposed by PWO 
*/

static int Reverse(StringCollection *SC)
{
	unsigned char **p, **q, *t;

	if (SC == NULL) {
		return NullPtrError("Reverse");
	}
	if (SC->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(SC,"Reverse");
	}
	if (SC->count < 2)
		return 1;

	p = SC->contents;
	q = &p[SC->count-1];
	while ( p < q ) {
		t = *p;
		*p = *q;
		*q = t;
		p++;
		q--;
	}
	SC->timestamp++;
	return 1;
}


StringCollectionInterface iStringCollection = {
    GetCount, GetFlags, SetFlags,  Clear,
	Contains,
	Erase,
    Finalize,
	Apply,
    Equal,
	Copy,
 	SetErrorFunction,
	Sizeof,
	newIterator,
	deleteIterator,
	Save,
	Load,
	GetElementSize,
	Add,
	PushFront,
	PopFront,
	InsertAt,
	RemoveAt,
	ReplaceAt,
	IndexOf,
	Sort,
	CastToArray,
	FindFirstText,
	FindNextText,
	FindText,
	FindTextIndex,
	FindTextPositions,
	WriteToFile,
	IndexIn,
	CreateFromFile,
	Create,
	CreateWithAllocator,
	AddRange,
	CopyTo,
	Insert,
	InsertIn,
	GetElement,
	GetCapacity,
	SetCapacity,
	SetCompareFunction,
	Reverse,
	Append,
	PopBack,
	PushBack,
	GetRange,
	GetAllocator,
	Mismatch,
	InitWithAllocator,
	Init,
};
