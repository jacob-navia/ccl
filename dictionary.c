/*------------------------------------------------------------------------
 Module:        D:\lcc\stl\ansic\dictionary.c
 Author:        jacob
 Project:
 State:
 Creation Date:
 Description:   This file implements the associative container  "Dictionary",
                that maps a character string to some arbitrary data.
                Dictionaries can copy their keys and data if the constructor
				is called with some element size. If the constructor is called
			    with element size zero, no storage will be used to copy the
				data, and the dictionary can store any kind of objects.
				According to the Boost terminology containers that copy are
				called "non-intrusive", and containers that do not copy are
				called "intrusive".
                (http://www.boost.org/doc/libs/1_40_0/doc/html/intrusive
				/intrusive_vs_nontrusive.html)
                If an element size is given, the software
                assumes that all data is of that size.

------------------------------------------------------------------------*/

#include <limits.h>
#include <stddef.h>
#include "containers.h"
#include "ccl_internal.h"
#include "assert.h"

static unsigned scatter[] = { /* Map characters to random values */
 	3376649973u, 2288603946u, 1954268477u, 2858154129u, 3254987376u,
 	1888560329u, 2079711150u, 1249903931u, 2056019508u, 3475721719u,
 	2183608578u, 1948585191u, 3510366957u,  479341015u,  137912281u,
 	1856397162u,  701025146u, 3777855647u, 3133726730u, 4113368641u,
 	 251772918u, 2859869442u,  824540103u,  614317204u, 3085688794u,
 	1104489690u, 3600905459u, 1036657084u, 1960148944u, 2441465117u,
 	3633092952u, 1202079507u, 1804386472u, 3798190281u, 2511419699u,
 	1032473403u, 3235883220u, 2593233477u, 2484192352u, 1834174643u,
 	3630460796u, 3436981729u,  876656665u, 1144446061u, 2179315054u,
 	2142937421u, 1163901871u,  703364539u, 1635510196u, 1558480853u,
 	3800782692u,  604753589u, 3558571372u,  274373881u,  183696063u,
 	4013401969u, 3787387983u,  551169993u, 2706792174u,  475596077u,
 	 784566245u, 2043924368u, 1567342084u, 3331009165u,  150886268u,
 	 596437426u, 2420547845u, 2898343441u, 1643521607u, 1387052253u,
 	 691524517u, 1709282085u, 2105726706u,  326318904u, 2270893751u,
 	1547094850u,  273913063u, 1180303327u, 1015098316u, 1122706416u,
 	1025137522u, 1445737386u, 3992079916u, 3230843455u, 3002906788u,
 	3543652723u, 1755107124u, 1921014418u,  683842306u, 2503306554u,
 	3688139822u, 3812611237u, 3363198012u, 1643682998u,  285631714u,
 	1910683492u, 4281003621u, 3709237568u, 2736065042u, 1422760317u,
 	 862182498u, 2248178396u, 3197393735u, 3974531276u,  157092128u,
 	3859796014u,  851355354u, 2511336234u, 3700246600u,  572627716u,
 	1519995253u,  342913937u,  328362706u, 3497158594u,  739312110u,
 	1482159142u, 4059308452u, 1115275813u, 2279798033u, 3563459711u,
 	 102382981u,  698626900u, 2506327534u, 2223405777u, 1827275406u,
 	 159038005u, 4159863896u, 3470995235u,  130302168u, 1077990744u,
 	1441602901u, 2757433577u,  200115595u,  993264331u, 2598999266u,
 	3842878136u, 3530540372u, 1361428823u,  248277624u, 1339695154u,
 	 432480863u, 2895143187u, 3166708344u, 2393286685u, 4271569970u,
 	 869342786u,  473223354u,  126812611u, 3904940903u, 1637555894u,
 	 996061127u, 1088298011u, 2952176066u, 2858912209u, 4228613491u,
 	4236158822u, 2582423590u, 2525024672u, 3677112391u, 3629698756u,
 	1496034522u, 2081171139u, 2352170546u,  176561938u, 3553901024u,
 	1142683711u, 2409311685u,  672560988u, 3693784086u,  689665476u,
 	1992869305u, 2102947696u, 1890679203u, 2387696458u, 1988263978u,
 	1536664131u,  768867302u, 2456175399u, 3136223828u,  202652382u,
 	4142812934u,  245277491u, 2630667112u,  240720193u, 2395371056u,
 	 707955862u, 4095017737u, 3236774548u, 3681653056u, 3285235880u,
 	 807411619u,  721125152u, 2671591148u, 4255706610u, 1694083953u,
 	3615121285u, 2744541524u, 2146568054u,  432941567u, 1070843254u,
 	2173029527u, 3630977578u, 3297023538u,   77429635u, 4131306785u,
 	1890732898u, 2010001485u, 1144304337u, 1673699809u, 1335369816u,
 	3596270401u, 3614930280u,  170584627u,  190006287u, 1491467787u,
 	 821380901u,  196708749u,  986375533u, 3133295550u, 2991205574u,
 	3983654535u, 3338932148u, 2374084740u, 4292366978u, 3657247497u,
 	3856158535u, 1497347358u, 3204988225u, 2733738804u, 1120807021u,
 	 450893717u, 2518878143u,   55245244u,  435713941u,  688959256u,
 	3878081060u, 3828717777u, 2111290183u, 3684667667u,  147090689u,
 	 671188737u, 1379556449u, 1326383789u, 1628432838u,  462410620u,
 	 544713991u, 1591539421u, 2938270133u, 1902128118u,  560215823u,
 	4293430683u, 1041753686u, 1365246147u, 2681506285u,  500008709u,
 	1129892475u
};

static const guid DictionaryGuid = {0xa334a9d, 0x897c, 0x4bed,
{0x92,0xa3,0x2,0xbf,0x86,0xd5,0x2e,0xcf}
};


struct _Dictionary {
	DictionaryInterface *VTable;
	size_t count;
	unsigned Flags;
	size_t size;
	ErrorFunction RaiseError;
	unsigned timestamp;
	size_t ElementSize;
	ContainerMemoryManager *Allocator;
	DestructorFunction DestructorFn;
	unsigned (*hash)(const unsigned char *Key);
	struct DataList {
		struct DataList *Next;
		unsigned char *Key;
		void *Value;
	} **buckets;
};
static Dictionary *Create(size_t elementsize,size_t hint);
/*------------------------------------------------------------------------
 Procedure:     hash ID:1
 Purpose:       Returns the hash code for a given character string.
                The characters are randomized through the
                razndomizer table. This is from the source code of
                lcc
 Input:         The character string to hash
 Output:        The hash code
 Errors:        None. Note that this function will NOT work for
                signed characters. The input MUST be unsigned
------------------------------------------------------------------------*/
#if 0
static unsigned hash(const unsigned char *str)
{
	unsigned int h = scatter[*str];
	if (*str == 0)
		return h;
	str++;
	while(*str) {
		h = (h >> 1) ^ scatter[*str++];
	}
	return h;
}
#endif

static int doerrorCall(ErrorFunction err,const char *fnName,int code)
{
	char buf[256];

	sprintf(buf,"iDictionary.%s",fnName);
	err(buf,code);
	return code;
}
static int ReadOnlyError(const Dictionary *SC,const char *fnName)
{
	return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_READONLY);
}
static int NullPtrError(const char *fnName)
{
	return doerrorCall(iError.RaiseError,fnName,CONTAINER_ERROR_BADARG);
}

static int BadArgError(const Dictionary *SC,const char *fnName)
{
	return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_BADARG);
}

static int NoMemoryError(Dictionary *SC,const char *fnName)
{
	return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_NOMEMORY);
}

static unsigned int hash(const unsigned char *key)
{
    unsigned int Hash = 0;
    const unsigned char *p;
	
    /*
	 * This was in the apache run time. This comment shows the long
	 * history of this function. Jacob N.
	 *
     * This is the popular `times 33' hash algorithm which is used by
     * perl and also appears in Berkeley DB. This is one of the best
     * known hash functions for strings because it is both computed
     * very fast and distributes very well.
     *
     * The originator may be Dan Bernstein but the code in Berkeley DB
     * cites Chris Torek as the source. The best citation I have found
     * is "Chris Torek, Hash function for text in C, Usenet message
     * <27038@mimsy.umd.edu> in comp.lang.c , October, 1990." in Rich
     * Salz's USENIX 1992 paper about INN which can be found at
     * <http://citeseer.nj.nec.com/salz92internetnews.html>.
     *
     * The magic of number 33, i.e. why it works better than many other
     * constants, prime or not, has never been adequately explained by
     * anyone. So I try an explanation: if one experimentally tests all
     * multipliers between 1 and 256 (as I did while writing a low-level
     * data structure library some time ago) one detects that even
     * numbers are not useable at all. The remaining 128 odd numbers
     * (except for the number 1) work more or less all equally well.
     * They all distribute in an acceptable way and this way fill a hash
     * table with an average percent of approx. 86%.
     *
     * If one compares the chi^2 values of the variants (see
     * Bob Jenkins ``Hashing Frequently Asked Questions'' at
     * http://burtleburtle.net/bob/hash/hashfaq.html for a description
     * of chi^2), the number 33 not even has the best value. But the
     * number 33 and a few other equally good numbers like 17, 31, 63,
     * 127 and 129 have nevertheless a great advantage to the remaining
     * numbers in the large set of possible multipliers: their multiply
     * operation can be replaced by a faster operation based on just one
     * shift plus either a single addition or subtraction operation. And
     * because a hash function has to both distribute good _and_ has to
     * be very fast to compute, those few numbers should be preferred.
     *
     *                  -- Ralf S. Engelschall <rse@engelschall.com>
     */
	
     for (p = key; *p; p++) {
        Hash = Hash * 33 + scatter[*p];
     }
	
    return Hash;
}

#if 0
static unsigned int hashlen(const unsigned char *key,int *klen)
{
    unsigned int hash = 0;
    const unsigned char *p;
	int i;
	
	for (p = key, i = *klen; i; i--, p++) {
		hash = hash * 33 + *p;
	}
    return hash;
}
#endif

/*------------------------------------------------------------------------
 Procedure:     GetElement ID:1
 Purpose:       Returns an element given its key
 Input:         The dictionary and the key
 Output:        The object is added, and the number of objects in
                the dictionary is returned. Zero or negative error
                code if there is an error.
 Errors:        If the dictionary is read only or the key is NULL an
                error code will be returned. Returns zero if there
                is no more memory
------------------------------------------------------------------------*/
static const void *GetElement(const Dictionary *Dict,const unsigned char *Key)
{
	size_t i;
	struct DataList *p;

	if (Dict == NULL || Key == NULL) {
		NullPtrError("GetElement");
		return NULL;
	}
	if (Dict->Flags & CONTAINER_READONLY) {
		ReadOnlyError(Dict,"GetElement");
		return NULL;
		
	}
	i = (*Dict->hash)(Key)%Dict->size;
	for (p = Dict->buckets[i]; p; p = p->Next)
		if (strcmp((char *)Key, (char *)p->Key) == 0)
			return p->Value;
	return NULL;
}

static int CopyElement(const Dictionary *Dict,const unsigned char *Key,void *outbuf)
{
	size_t i;
	struct DataList *p;
	
	if (Dict == NULL) {
		return NullPtrError("CopyElement");
	}
	if (Key == NULL)
		return BadArgError(Dict,"CopyElement");
	i = (*Dict->hash)(Key)%Dict->size;
	for (p = Dict->buckets[i]; p; p = p->Next)
		if (strcmp((char *)Key, (char *)p->Key) == 0) {
			memcpy(outbuf,p->Value,Dict->ElementSize);
			return 1;
		}
	return 0;
}

static int Contains(Dictionary *Dict,const unsigned char *Key)
{
	if (Dict == NULL)  {
		return NullPtrError("Contains");
	}
	if (Key == NULL)
		return BadArgError(Dict,"Contains");
	return GetElement(Dict,Key) ? 1 : 0;
}

static int Equal(const Dictionary *d1,const Dictionary *d2)
{
	struct DataList *p1,*p2;
	size_t i;
	if (d1 == d2) return 1;
	if (d1 == NULL || d2 == NULL)
		return 0;
	if (d1->count != d2->count || d1->Flags != d2->Flags)
		return 0;
	if (d1->size != d2->size || d1->hash != d2->hash)
		return 0;
	if (d1->ElementSize != d2->ElementSize)
		return 0;
	for (i=0; i < d1->size;i++) {
		size_t j = 0;
		p1 = d1->buckets[i]; p2 = d2->buckets[i];
		while (p1 && p2) {
			if (strcmp((char *)p1->Key,(char *)p2->Key))
				return 0;
			if (memcmp(p1->Value,p2->Value,d1->ElementSize))
				return 0;
			j++;
			p1 = p1->Next; p2 = p2->Next;
		}
		if (p1 != p2)
			return 0;
	}
	return 1;
}
/*------------------------------------------------------------------------
 Procedure:     Add ID:1
 Purpose:       Adds one entry to the dictionary. If another entry
                exists for the same key it will be replaced.
 Input:         The dictionary, the key, and the value to be added
 Output:        The number of items in the dictionary or a negative
                error code
 Errors:        The container must be read/write.
------------------------------------------------------------------------*/
static int Add(Dictionary *Dict,const unsigned char *Key,void *Value)
{
	size_t i;
	struct DataList *p;
	unsigned char *tmp;

	if (Dict == NULL) 
		return NullPtrError("Add");
	if (Dict->Flags & CONTAINER_READONLY) 
		return ReadOnlyError(Dict,"Add");
	if (Key == NULL || Value == NULL) 
		return BadArgError(Dict,"Add");

	i = (*Dict->hash)(Key)%Dict->size;
	for (p = Dict->buckets[i]; p; p = p->Next) {
		if (strcmp((char *)Key, (char *)p->Key) == 0)
			break;
	}
	Dict->timestamp++;
	if (p == NULL) {
		/* Allocate both value and key to avoid leaving the
		   container in an invalid state if a second allocation fails */
		p = Dict->Allocator->malloc(sizeof(*p)+Dict->ElementSize);
		tmp = Dict->Allocator->malloc(1+strlen((char *)Key));
		if (p == NULL || tmp == NULL) {
			if (p) Dict->Allocator->free(p);
			if (tmp) Dict->Allocator->free(tmp);
			return NoMemoryError(Dict,"Add");
		}
        p->Value = (void *)(p+1);
		memcpy((void *)p->Value,Value,Dict->ElementSize);
		strcpy((char *)tmp,(char *)Key);
		p->Key = tmp;
		i = (*Dict->hash)(Key)%Dict->size;
		p->Next = Dict->buckets[i];
		Dict->buckets[i] = p;
		Dict->count++;
		return 1;
	}
	/* Overwrite the data for an existing element */
	memcpy((void *)p->Value,Value,Dict->ElementSize);
	return 0;
}
static int Insert(Dictionary *Dict,const unsigned char *Key,void *Value)
{
	size_t i;
	struct DataList *p;
	unsigned char *tmp;

	if (Dict == NULL) 
		return NullPtrError("Add");
	if (Dict->Flags & CONTAINER_READONLY) 
		return ReadOnlyError(Dict,"Add");
	if (Key == NULL || Value == NULL) 
		return BadArgError(Dict,"Add");

	i = (*Dict->hash)(Key)%Dict->size;
	for (p = Dict->buckets[i]; p; p = p->Next) {
		if (strcmp((char *)Key, (char *)p->Key) == 0)
			break;
	}
	if (p)
		return 0;
	Dict->timestamp++;
	/* Allocate both value and key to avoid leaving the
		container in an invalid state if a second allocation fails */
	p = Dict->Allocator->malloc(sizeof(*p)+Dict->ElementSize);
	tmp = Dict->Allocator->malloc(1+strlen((char *)Key));
	if (p == NULL || tmp == NULL) {
		if (p) Dict->Allocator->free(p);
		if (tmp) Dict->Allocator->free(tmp);
		return NoMemoryError(Dict,"Add");
	}
    p->Value = (void *)(p+1);
	memcpy((void *)p->Value,Value,Dict->ElementSize);
	strcpy((char *)tmp,(char *)Key);
	p->Key = tmp;
	p->Next = Dict->buckets[i];
	Dict->buckets[i] = p;
	Dict->count++;
	return 1;
}
static int Replace(Dictionary *Dict,const unsigned char *Key,void *Value)
{
	size_t i;
	struct DataList *p;

	if (Dict == NULL) {
		return NullPtrError("Replace");
	}
	if (Dict->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(Dict,"Replace");
	}
	if (Key == NULL || Value == NULL) {
		return BadArgError(Dict,"Replace");
	}

	i = (*Dict->hash)(Key)%Dict->size;
	for (p = Dict->buckets[i]; p; p = p->Next) {
		if (strcmp((char *)Key, (char *)p->Key) == 0)
			break;
	}
	if (p == NULL) {
		return CONTAINER_ERROR_NOTFOUND;
	}
	Dict->timestamp++;
	if (Dict->DestructorFn)
		Dict->DestructorFn(p->Value);
	/* Overwrite the data for an existing element */
	memcpy((void *)p->Value,Value,Dict->ElementSize);
	return 1;
}

/*------------------------------------------------------------------------
 Procedure:     Size ID:1
 Purpose:       Returns the number of entries in the dictionary
 Input:         The dictionary
 Output:        The number of entries
 Errors:        None
------------------------------------------------------------------------*/
static size_t Size(Dictionary *Dict)
{
	if (Dict == NULL) {
		NullPtrError("Size");
		return 0; 
	}
	return Dict->count;
}

/*------------------------------------------------------------------------
 Procedure:     GetFlags ID:1
 Purpose:       Returns the flags of the given dictionary
 Input:         The dictionary
 Output:        The flags
 Errors:        None
------------------------------------------------------------------------*/
static unsigned GetFlags(Dictionary *Dict)
{
	if (Dict == NULL) {
		NullPtrError("GetFlags");
		return 0; 
	}
	return Dict->Flags;
}

static size_t Sizeof(Dictionary *dict)
{
	if (dict == NULL) {
		return sizeof(Dictionary); 
	}	
	return dict->ElementSize * dict->count + sizeof(dict) + dict->count*sizeof(struct DataList);
}


/*------------------------------------------------------------------------
 Procedure:     SetFlags ID:1
 Purpose:       Sets the flags field of a dictionary
 Input:         The dictionary and the flags to set
 Output:        Returns the old value of the flags
 Errors:        None
------------------------------------------------------------------------*/
static unsigned SetFlags(Dictionary *Dict,unsigned Flags)
{
	unsigned oldFlags;
	if (Dict == NULL) {
		NullPtrError("SetFlags");
		return 0; 
	}	
	oldFlags = Dict->Flags;
	Dict->Flags = Flags;
	return oldFlags;
}


/*------------------------------------------------------------------------
 Procedure:     Apply ID:1
 Purpose:       Calls the given function for each entry in the
                dictionary.
 Input:         The dictionary, the function to call and an extra
                argument to be passed to the called function at each
                item.
 Output:        1 if no errors, zero otherwise
 Errors:        If Dictionary is a NULL pointer or the function is a
                NULL pointer returns zero.
------------------------------------------------------------------------*/
static int Apply(Dictionary *Dict,int (*apply)(const unsigned char *Key,const void *Value, void *ExtraArgs),
	void *ExtraArgs)
{
	size_t i;
	unsigned stamp;
	struct DataList *p;

	if (Dict == NULL) {
		return	NullPtrError("Apply");
	}
	if (apply == NULL)
		return BadArgError(Dict,"Apply");
	stamp = Dict->timestamp;
	for (i = 0; i < Dict->size; i++) {
		for (p = Dict->buckets[i]; p; p = p->Next) {
			apply(p->Key,p->Value, ExtraArgs);
			if (Dict->timestamp != stamp)
				return 0;
		}
	}
	return 1;
}

static int InsertIn(Dictionary *dst,Dictionary *src)
{
	size_t i;
	unsigned stamp;
	int r;
	struct DataList *p;

	if (dst == NULL) {
		return NullPtrError("InsertIn");
	}
	if (src == NULL)
		return BadArgError(dst,"InsertIn");

	if (dst->Flags& CONTAINER_READONLY) 
		return ReadOnlyError(dst,"InsertIn");

	if (src->ElementSize != dst->ElementSize) {
		dst->RaiseError("iDictionary.InsertIn",CONTAINER_ERROR_INCOMPATIBLE);
		return CONTAINER_ERROR_INCOMPATIBLE;
	}
	stamp = src->timestamp;
	for (i = 0; i < src->size; i++) {
		for (p = src->buckets[i]; p; p = p->Next) {
			r = Add(dst,p->Key,p->Value);
			if (r < 0)
				return r;
			if (src->timestamp != stamp)
				return 0;
		}
	}
	return 1;
}
/*------------------------------------------------------------------------
 Procedure:     Remove ID:1
 Purpose:       Erases an entry from the dictionary if the entry
                exists
 Input:         The dictionary and the key to erase
 Output:        Returns 1 if the key was found and erased, zero
                otherwise.
 Errors:        If the dictionary pointer is NULL or the key is NULL
                returns zero. If the dictionary is read-only the
				result is also zero.
------------------------------------------------------------------------*/
static int Erase(Dictionary *Dict,const unsigned char *Key)
{
	size_t i;
	struct DataList **pp;

	if (Dict == NULL) 
		return NullPtrError("Erase");

	if (Key == NULL) 
		return BadArgError(Dict,"Erase");

	if (Dict->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(Dict,"Erase");
	}
	i = (*Dict->hash)(Key)%Dict->size;
	for (pp = &Dict->buckets[i]; *pp; pp = &(*pp)->Next) {
		if (strcmp((char *)Key, (char *)(*pp)->Key) == 0) {
			struct DataList *p = *pp;
			*pp = p->Next;
			if (Dict->DestructorFn)
				Dict->DestructorFn(p->Value);
			Dict->Allocator->free(p->Key);
			Dict->Allocator->free(p);
			Dict->count--;
			Dict->timestamp++;
			return 1;
		}
	}
	return CONTAINER_ERROR_NOTFOUND;
}
/*------------------------------------------------------------------------
 Procedure:     Clear ID:1
 Purpose:       Clear all the entries in the given dictionary
 Input:         The dictionary to clear
 Output:        1 if succeeds, zero otherwise
 Errors:        The dictionary must be read/write
------------------------------------------------------------------------*/
static int Clear(Dictionary *Dict)
{
	if (Dict == NULL) {
		return NullPtrError("Clear");
	}
	if (Dict->Flags & CONTAINER_READONLY) {
		return ReadOnlyError(Dict,"Clear");
	}
	if (Dict->count > 0) {
		size_t i;
		struct DataList *p, *q;
		for (i = 0; i < Dict->size; i++)
			for (p = Dict->buckets[i]; p; p = q) {
				q = p->Next;
				if (Dict->DestructorFn)
					Dict->DestructorFn(p->Value);
				Dict->Allocator->free(p->Key);
				Dict->Allocator->free(p);
			}
	}
	memset(Dict->buckets,0,Dict->size*sizeof(void *));
	Dict->count=0;
	return 1;
}

/*------------------------------------------------------------------------
 Procedure:     Finalize ID:1
 Purpose:       Clears all memory used by a dictionary and destroys
                it.
 Input:         The dictionary to destroy
 Output:        Returns 1 for success, zero otherwise
 Errors:        Same as Clear: dictionary must be read/write
------------------------------------------------------------------------*/
static int Finalize(Dictionary *Dict)
{
	int r = Clear(Dict);
	if (0 > r)
		return r;
	Dict->Allocator->free(Dict->buckets);
	Dict->Allocator->free(Dict);
	return 1;
}

static ErrorFunction SetErrorFunction(Dictionary *Dict,ErrorFunction fn)
{
	ErrorFunction old;
	if (Dict == NULL) {
		NullPtrError("SetErrorFunction");
		return 0; 
	}	
	old = Dict->RaiseError;
	if (fn)
		Dict->RaiseError = fn;
	return old;
}

/*------------------------------------------------------------------------
 Procedure:     GetKeys ID:1
 Purpose:       Returns all keys stored in the dictionary in a
                string collection object.
 Input:         The dictionary
 Output:        A string collection with all keys
 Errors:        NULL if no more memory available
------------------------------------------------------------------------*/
static StringCollection *GetKeys(Dictionary *Dict)
{
	size_t i;
	struct DataList *p;
	StringCollection *result;

	if (Dict == NULL) {
		NullPtrError("GetKeys");
		return 0; 
	}	

	result = iStringCollection.Create(Dict->count);
	if (result == NULL)
		return NULL;
	for (i=0; i<Dict->size;i++) {
		for (p = Dict->buckets[i]; p; p = p->Next) {
			iStringCollection.Add(result,p->Key);
		}
	}
	return result;
}

/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */

struct DictionaryIterator {
	Iterator it;
	Dictionary *Dict;
	size_t index;
	struct DataList *dl;
	size_t timestamp;
	unsigned long Flags;
};

static void *GetNext(Iterator *it)
{
	struct DictionaryIterator *d = (struct DictionaryIterator *)it;
	Dictionary *Dict;
	void *retval;

	if (it == NULL) {
		NullPtrError("GetNext");
		return NULL;
	}
	Dict = d->Dict;
	if (d->timestamp != Dict->timestamp) {
		Dict->RaiseError("iDictionary.GetNext",CONTAINER_ERROR_OBJECT_CHANGED);
		return NULL;
	}
	if (d->index >= Dict->size)
		return NULL;
	while (Dict->buckets[d->index] == NULL) {
		d->index++;
		if (d->index >= Dict->size)
			return NULL;
	}
	if (d->dl == NULL) {
		d->dl = Dict->buckets[d->index];
	}
	retval = d->dl->Value;
	d->dl = d->dl->Next;
	if (d->dl == NULL) {
		d->index++;
	}
	return retval;
}

static void *GetFirst(Iterator *it)
{
	struct DictionaryIterator *Dicti = (struct DictionaryIterator *)it;

	if (it == NULL) {
		NullPtrError("GetFirt");
		return NULL;
	}
	if (Dicti->Dict->count == 0)
		return NULL;
	Dicti->index = 0;
	Dicti->dl = NULL;
	return GetNext(it);
}

static Iterator *newIterator(Dictionary *Dict)
{
	struct DictionaryIterator *result;

	if (Dict == NULL) {
		NullPtrError("newIterator");
		return NULL;
	}

	result = Dict->Allocator->malloc(sizeof(struct DictionaryIterator));
	if (result == NULL) {
		NoMemoryError(Dict,"newIterator");
		return NULL;
	}
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetNext;
	result->it.GetFirst = GetFirst;
	result->Dict = Dict;
	result->timestamp = Dict->timestamp;
	return &result->it;
}

static int deleteIterator(Iterator *it)
{
	struct DictionaryIterator *d = (struct DictionaryIterator *)it;
	Dictionary *Dict;
	if (d ==NULL) {
		return NullPtrError("deleteIterator");
	}
	Dict = d->Dict;
	Dict->Allocator->free(it);
	return 1;
}

static Vector *CastToArray(Dictionary *Dict)
{
	size_t i;
	struct DataList *p;
	Vector *result;
	
	if (Dict == NULL) {
		NullPtrError("CastToArray");
		return NULL;
	}
	result = iVector.Create(Dict->ElementSize,Dict->count);

	for (i=0; i<Dict->size;i++) {
		for (p = Dict->buckets[i]; p; p = p->Next) {
			iVector.Add(result,(char *)p->Value);
		}
	}
	return result;
}

static int Save(Dictionary *Dict,FILE *stream, SaveFunction saveFn,void *arg)
{
	Vector *al; 
	StringCollection *sc; 
	
	if (Dict == NULL) {
		return NullPtrError("Save");
	}
	if (stream == NULL) {
		return BadArgError(Dict,"Save");
	}
	al = CastToArray(Dict);
	sc = GetKeys(Dict);
	if (fwrite(&DictionaryGuid,sizeof(guid),1,stream) <= 0)
		return EOF;
	if (iStringCollection.Save(sc,stream,NULL,NULL) < 0)
		return EOF;
	if (iVector.Save(al,stream,saveFn,arg) < 0)
		return EOF;
	return 0;
}
static Dictionary *Copy(Dictionary *src)
{
	Dictionary *result;
	size_t i;
	struct DataList *rvp;
	
	if (src == NULL) {
		NullPtrError("Copy");
		return NULL;
	}
	result = Create(src->ElementSize, src->count);
	if (result == NULL) {
		NoMemoryError(src,"Copy");
		return NULL;
	}
	result->Flags = src->Flags;
	result->hash = src->hash;
	result->RaiseError = src->RaiseError;
	for (i=0; i<src->size;i++) {
		rvp = src->buckets[i];
		while (rvp) {
			result->VTable->Add(result,rvp->Key,rvp->Value);
			rvp = rvp->Next;
		}
	}
	return result;
}

static Dictionary *Load(FILE *stream, ReadFunction readFn, void *arg)
{
	StringCollection *sc;
	Vector *al;
	Dictionary *result;
	size_t i;
	guid Guid;

	if (stream == NULL) {
		NullPtrError("Load");
		return NULL;
	}
	if (fread(&Guid,sizeof(guid),1,stream) <= 0) {
		iError.RaiseError("iDictionary.Load",CONTAINER_ERROR_FILE_READ);
		return NULL;
	}
	if (memcmp(&Guid,&DictionaryGuid,sizeof(guid))) {
		iError.RaiseError("iDictionary.Load",CONTAINER_ERROR_WRONGFILE);
		return NULL;
	}
	sc = iStringCollection.Load(stream,NULL,NULL);
	if (sc == NULL)
		return NULL;
	al = iVector.Load(stream,readFn,arg);
	if (al == NULL) {
		iStringCollection.Finalize(sc);
		return NULL;
	}
	result = Create(iVector.GetElementSize(al),iStringCollection.Size(sc));
	for (i=0; i<iStringCollection.Size(sc);i++) {
		unsigned char *key = (unsigned char *)iStringCollection.GetElement(sc,i);
		void *data = iVector.GetElement(al,i);
		result->VTable->Add(result,key,data);
	}
	iStringCollection.Finalize(sc);
	iVector.Finalize(al);
	return result;
}

static size_t GetElementSize(Dictionary *d) 
{
	if (d == NULL) {
		NullPtrError("GetElementSize");
		return 0;
	}
	return d->ElementSize;
}

static ContainerMemoryManager *GetAllocator(Dictionary *AL)
{
	if (AL == NULL) {
		return NULL;
	}
	return AL->Allocator;
}


/*------------------------------------------------------------------------
 Procedure:     newDictionary ID:1
 Purpose:       Constructs a new dictionary object and initializes
				all fields
 Input:         A hint for the size of the number of elements this
 table will hold
 Output:        A pointer to a newly allocated table
 Errors:        If no more memory is available returns NULL
 ------------------------------------------------------------------------*/
#undef roundup
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))
static Dictionary *InitWithAllocator(Dictionary *Dict,size_t elementsize,size_t hint,ContainerMemoryManager *allocator)
{
	size_t i,allocSiz;
	static unsigned primes[] = { 509, 509, 1021, 2053, 4093, 8191, 16381, 
		32771, 65521, 131071, 0 };
	for (i = 1; primes[i] < hint && primes[i] > 0; i++)
		;
	allocSiz = sizeof (Dictionary);
	memset(Dict,0,allocSiz);
	allocSiz = primes[i-1]*sizeof (Dict->buckets[0]);
	Dict->buckets = allocator->malloc(allocSiz);
	if (Dict->buckets == NULL) {
		return NULL;
	}
	memset(Dict->buckets,0,allocSiz);
	Dict->size = primes[i-1];
	Dict->hash = hash;
	Dict->VTable = &iDictionary;
	Dict->ElementSize = elementsize;
	Dict->Allocator = allocator;
	Dict->RaiseError = iError.RaiseError;
	return Dict;
}

static Dictionary *Init(Dictionary *dict,size_t elementsize,size_t hint)
{
	return InitWithAllocator(dict, elementsize, hint, CurrentMemoryManager);
}

static Dictionary *CreateWithAllocator(size_t elementsize,size_t hint,ContainerMemoryManager *allocator)
{
	Dictionary *Dict,*result;

	size_t allocSiz = sizeof (Dictionary);
	Dict = allocator->malloc(allocSiz);
	if (Dict == NULL) {
		iError.RaiseError("iDictionary.Create",CONTAINER_ERROR_NOMEMORY);
		return NULL;
	}
	result = InitWithAllocator(Dict,elementsize,hint,allocator);
	if (result == NULL)
		allocator->free(Dict);
	return result;
}
static Dictionary *Create(size_t elementsize,size_t hint)
{
	return CreateWithAllocator(elementsize,hint,CurrentMemoryManager);
}
static DestructorFunction SetDestructor(Dictionary *cb,DestructorFunction fn)
{
	DestructorFunction oldfn;
	if (cb == NULL)
		return NULL;
	oldfn = cb->DestructorFn;
	if (fn)
		cb->DestructorFn = fn;
	return oldfn;
}
							   
DictionaryInterface iDictionary = {
	Size,
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
	newIterator,
	deleteIterator,
	Save,
	Load,
	GetElementSize,

	Add,
	GetElement,
	Replace,
	Insert,
	CastToArray,
	CopyElement,
	InsertIn,
	Create,
	CreateWithAllocator,
	Init,
	InitWithAllocator,
	GetKeys,
	GetAllocator,
	SetDestructor,
};
