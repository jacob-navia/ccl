/*------------------------------------------------------------------------
 Module:        dictionarygen.c
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


static DATA_TYPE *Create(size_t elementsize,size_t hint);
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
static unsigned hash(const char *str)
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

    snprintf(buf,sizeof(buf),"iDictionary.%s",fnName);
    err(buf,code);
    return code;
}
static int ReadOnlyError(const DATA_TYPE *SC,const char *fnName)
{
    return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_READONLY);
}
static int NullPtrError(const char *fnName)
{
    return doerrorCall(iError.RaiseError,fnName,CONTAINER_ERROR_BADARG);
}

static int BadArgError(const DATA_TYPE *SC,const char *fnName)
{
    return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_BADARG);
}

static int NoMemoryError(const DATA_TYPE *SC,const char *fnName)
{
    return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_NOMEMORY);
}

static size_t hash(const CHARTYPE *key)
{
    size_t Hash = 0;
    const CHARTYPE *p;

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

    for (p = (const CHARTYPE *)key; *p; p++) {
        Hash = Hash * 33 + scatter[(unsigned)(*p)&255];
    }

    return Hash;
}

#if 0
static unsigned int hashlen(const char *key,int *klen)
{
    unsigned int hash = 0;
    const char *p;
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
static void *GetElement(const DATA_TYPE *Dict,const CHARTYPE *Key)
{
    size_t i;
    struct DATALIST *p;

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
        if (STRCMP(Key, p->Key) == 0)
            return Dict->ElementSize ? p->Value : p->Key;
    return NULL;
}

static int CopyElement(const DATA_TYPE *Dict,const CHARTYPE *Key,void *outbuf)
{
    size_t i;
    struct DATALIST *p;

    if (Dict == NULL) {
        return NullPtrError("CopyElement");
    }
    if (Key == NULL)
        return BadArgError(Dict,"CopyElement");

    if (Dict->ElementSize == 0) return 0;
    i = (*Dict->hash)(Key)%Dict->size;

    for (p = Dict->buckets[i]; p; p = p->Next)
        if (STRCMP(Key, p->Key) == 0) {
            if (outbuf != NULL && Dict->ElementSize != 0)
                memcpy(outbuf,p->Value,Dict->ElementSize);
            return 1;
        }
    return 0;
}

static int Contains(const DATA_TYPE *Dict,const CHARTYPE *Key)
{
    if (Dict == NULL)  {
        return NullPtrError("Contains");
    }
    if (Key == NULL)
        return BadArgError(Dict,"Contains");
    return GetElement(Dict,Key) ? 1 : 0;
}

static int Equal(const DATA_TYPE *d1,const DATA_TYPE *d2)
{
    struct DATALIST *p1,*p2;
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
            if (STRCMP((CHARTYPE *)p1->Key,(CHARTYPE *)p2->Key))
                return 0;
            if (d1->ElementSize &&
                memcmp(p1->Value,p2->Value,d1->ElementSize))
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
static int add_nd(DATA_TYPE *Dict,const CHARTYPE *Key,const void *Value,int is_insert)
{
    size_t i;
    struct DATALIST *p;
    CHARTYPE *tmp;
    int result = 1;

    i = (*Dict->hash)(Key)%Dict->size;
    for (p = Dict->buckets[i]; p; p = p->Next) {
        if (STRCMP(Key, p->Key) == 0)
            break;
    }
    if (p && is_insert) return 0;
    Dict->timestamp++;
    if (p == NULL) {
        /* Allocate both value and key to avoid leaving the
        container in an invalid state if a second allocation fails */
        p = Dict->Allocator->malloc(sizeof(*p)+Dict->ElementSize);
        tmp = Dict->Allocator->malloc(sizeof(CHARTYPE)*(1+STRLEN(Key)));
        if (p == NULL || tmp == NULL) {
            if (p) Dict->Allocator->free(p);
            if (tmp) Dict->Allocator->free(tmp);
            return NoMemoryError(Dict,"Add");
        }
        if (Value && Dict->ElementSize) {
            p->Value = (void *)(p+1);
            memcpy((void *)p->Value,Value,Dict->ElementSize);
        }
        else if (Dict->ElementSize == 0)
            p->Value = tmp;
        else p->Value = NULL;
        STRCPY(tmp,Key);
        p->Key = tmp;
        p->Next = Dict->buckets[i];
        Dict->buckets[i] = p;
        Dict->count++;
    }
    else {
        /* Overwrite the data for an existing element */
        if (Value && Dict->ElementSize)
            memcpy((void *)p->Value,Value,Dict->ElementSize);
        else if (Dict->ElementSize) p->Value = NULL;
        result = 0;
    }

    return result;
}
static int Add(DATA_TYPE *Dict,const CHARTYPE *Key,const void *Value)
{
    int result;

    if (Dict == NULL)
        return NullPtrError("Add");
    if (Dict->Flags & CONTAINER_READONLY)
        return ReadOnlyError(Dict,"Add");
    if (Key == NULL)
        return BadArgError(Dict,"Add");

    result = add_nd(Dict,Key,Value,0);
    if (result >= 0 && (Dict->Flags & CONTAINER_HAS_OBSERVER))
        iObserver.Notify(Dict,CCL_ADD,Value,NULL);
    return result;
}
static int Insert(DATA_TYPE *Dict,const CHARTYPE *Key,const void *Value)
{
    int result;

    if (Dict == NULL)
        return NullPtrError("Insert");
    if (Dict->Flags & CONTAINER_READONLY)
        return ReadOnlyError(Dict,"Insert");
    if (Key == NULL || (Value == NULL && Dict->ElementSize > 0))
        return BadArgError(Dict,"Insert");

    result = add_nd(Dict,Key,Value,1);
    if (result >= 0 && (Dict->Flags & CONTAINER_HAS_OBSERVER))
        iObserver.Notify(Dict,CCL_INSERT,Value,NULL);
    return result;
}
static int Replace(DATA_TYPE *Dict,const CHARTYPE *Key,const void *Value)
{
    size_t i;
    struct DATALIST *p;

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
        if (STRCMP((CHARTYPE *)Key, (CHARTYPE *)p->Key) == 0)
            break;
    }
    if (p == NULL) {
        return CONTAINER_ERROR_NOTFOUND;
    }
    if (Dict->ElementSize == 0)
        return 1;
    if (Dict->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(Dict,CCL_REPLACE,p->Value,Value);
    Dict->timestamp++;
    if (Dict->DestructorFn)
        Dict->DestructorFn(p->Value);
    /* Overwrite the data for an existing element */
    if (Value)
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
static size_t Size(const DATA_TYPE *Dict)
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
static unsigned GetFlags(const DATA_TYPE *Dict)
{
    if (Dict == NULL) {
        NullPtrError("GetFlags");
        return 0;
    }
    return Dict->Flags;
}

static size_t Sizeof(const DATA_TYPE *dict)
{
    if (dict == NULL) {
        return sizeof(Dictionary);
    }
    return dict->ElementSize * dict->count + sizeof(dict) + dict->count*sizeof(struct DATALIST);
}


/*------------------------------------------------------------------------
 Procedure:     SetFlags ID:1
 Purpose:       Sets the flags field of a dictionary
 Input:         The dictionary and the flags to set
 Output:        Returns the old value of the flags
 Errors:        None
------------------------------------------------------------------------*/
static unsigned SetFlags(DATA_TYPE *Dict,unsigned Flags)
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
static int Apply(DATA_TYPE *Dict,int (*apply)(const CHARTYPE *Key,const void *Value, void *ExtraArgs),
    void *ExtraArgs)
{
    size_t i;
    unsigned stamp;
    struct DATALIST *p;

    if (Dict == NULL) {
        return	NullPtrError("Apply");
    }
    if (apply == NULL)
        return BadArgError(Dict,"Apply");
    stamp = Dict->timestamp;
    for (i = 0; i < Dict->size; i++) {
        for (p = Dict->buckets[i]; p; p = p->Next) {
            if (Dict->ElementSize)
                apply(p->Key,p->Value, ExtraArgs);
            else apply(p->Key,NULL,ExtraArgs);
            if (Dict->timestamp != stamp)
                return 0;
        }
    }
    return 1;
}

static int InsertIn(DATA_TYPE *dst,DATA_TYPE *src)
{
    size_t i;
    unsigned stamp;
    int r;
    struct DATALIST *p;

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
            r = add_nd(dst,p->Key,p->Value,0);
            if (r < 0)
                return r;
            if (src->timestamp != stamp)
                return 0;
        }
    }
    if (dst->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(dst,CCL_INSERT_IN,src,NULL);
    return 1;
}
/*------------------------------------------------------------------------
 Procedure:     Erase ID:1
 Purpose:       Erases an entry from the dictionary if the entry
                exists
 Input:         The dictionary and the key to erase
 Output:        Returns 1 if the key was found and erased, zero
                otherwise.
 Errors:        If the dictionary pointer is NULL or the key is NULL
                returns zero. If the dictionary is read-only the
                result is also zero.
------------------------------------------------------------------------*/
static int Erase(DATA_TYPE *Dict,const CHARTYPE *Key)
{
    size_t i;
    struct DATALIST **pp;

    if (Dict == NULL)
        return NullPtrError("Erase");

    if (Key == NULL)
        return BadArgError(Dict,"Erase");

    if (Dict->Flags & CONTAINER_READONLY) {
        return ReadOnlyError(Dict,"Erase");
    }
    i = (*Dict->hash)(Key)%Dict->size;
    for (pp = &Dict->buckets[i]; *pp; pp = &(*pp)->Next) {
        if (STRCMP(Key, (*pp)->Key) == 0) {
            struct DATALIST *p = *pp;
            if (Dict->Flags & CONTAINER_HAS_OBSERVER)
                iObserver.Notify(Dict,CCL_ERASE_AT,p->Key,p->Value);

            *pp = p->Next;
            if (Dict->DestructorFn && Dict->ElementSize)
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
static int Clear(DATA_TYPE *Dict)
{

    if (Dict == NULL) {
        return NullPtrError("Clear");
    }
    if (Dict->Flags & CONTAINER_READONLY) {
        return ReadOnlyError(Dict,"Clear");
    }
    if (Dict->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(Dict,CCL_CLEAR,NULL,NULL);
    if (Dict->count > 0) {
        size_t i;
        struct DATALIST *p, *q;
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
static int Finalize(DATA_TYPE *Dict)
{

    int r = Clear(Dict);
    if (0 > r)
        return r;
    if (Dict->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(Dict,CCL_FINALIZE,NULL,NULL);
    if (Dict->VTable != &EXTERNAL_NAME)
        Dict->Allocator->free(Dict->VTable);
    Dict->Allocator->free(Dict->buckets);
    Dict->Allocator->free(Dict);
    return 1;
}

static ErrorFunction SetErrorFunction(DATA_TYPE *Dict,ErrorFunction fn)
{
    ErrorFunction old;
    if (Dict == NULL) { return iError.RaiseError; }
    old = Dict->RaiseError;
    if (fn) Dict->RaiseError = fn;
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
static STRCOLLECTION *GetKeys(const DATA_TYPE *Dict)
{
    size_t i;
    struct DATALIST *p;
    STRCOLLECTION *result;

    if (Dict == NULL) {
        NullPtrError("GetKeys");
        return 0;
    }

    result = iSTRCOLLECTION.Create(Dict->count);
    if (result == NULL)
        return NULL;
    for (i=0; i<Dict->size;i++) {
        for (p = Dict->buckets[i]; p; p = p->Next) {
            iSTRCOLLECTION.Add(result,p->Key);
        }
    }
    return result;
}
/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */
static size_t GetPosition(Iterator *it)
{
    struct ITERATOR *d = (struct ITERATOR *)it;
    return d->index;
}

static void *GetNext(Iterator *it)
{
    struct ITERATOR *d = (struct ITERATOR *)it;
    DATA_TYPE *Dict;
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
    if (d->Dict->ElementSize == 0)
        retval = d->dl->Key;
    else
        retval = d->dl->Value;
    d->dl = d->dl->Next;
    if (d->dl == NULL) {
        d->index++;
    }
    return retval;
}

static void *GetFirst(Iterator *it)
{
    struct ITERATOR *Dicti = (struct ITERATOR *)it;

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

static int ReplaceWithIterator(Iterator *it, void *data,int direction)
{
    struct ITERATOR *li = (struct ITERATOR *)it;
    int result=1;
    struct DATALIST *dl;

    if (it == NULL) {
        return NullPtrError("Replace");
    }
    if (li->Dict->Flags & CONTAINER_READONLY) {
        li->Dict->RaiseError("Replace",CONTAINER_ERROR_READONLY);
        return CONTAINER_ERROR_READONLY;
    }

    if (li->Dict->count == 0)
        return 0;
    if (li->timestamp != li->Dict->timestamp) {
        li->Dict->RaiseError("Replace",CONTAINER_ERROR_OBJECT_CHANGED);
        return CONTAINER_ERROR_OBJECT_CHANGED;
    }
    dl = li->dl;
    GetNext(it);
    if (data == NULL)
        result = Erase(li->Dict, dl->Key);
    else if (li->Dict->ElementSize) {
        memcpy(dl->Value,data,li->Dict->ElementSize);
        result = 1;
    }
    if (result >= 0) {
        li->timestamp = li->Dict->timestamp;
    }
    return result;
}

static void *Seek(Iterator *it, size_t idx)
{
    return NULL;
}

static Iterator *NewIterator(DATA_TYPE *Dict)
{
    struct ITERATOR *result;

    if (Dict == NULL) {
        NullPtrError("NewIterator");
        return NULL;
    }

    result = Dict->Allocator->malloc(sizeof(struct ITERATOR));
    if (result == NULL) {
        NoMemoryError(Dict,"NewIterator");
        return NULL;
    }
    result->it.GetNext = GetNext;
    result->it.GetPrevious = GetNext;
    result->it.GetFirst = GetFirst;
    result->Dict = Dict;
    result->it.Replace = ReplaceWithIterator;
    result->it.GetPosition = GetPosition;
    result->it.Seek = Seek;
    result->timestamp = Dict->timestamp;
    return &result->it;
}

static int InitIterator(DATA_TYPE *Dict,void *buf)
{
    struct ITERATOR *result = buf;

    if (Dict == NULL || buf == NULL) {
        NullPtrError("NewIterator");
        return CONTAINER_ERROR_BADARG;
    }

    result->it.GetNext = GetNext;
    result->it.GetPrevious = GetNext;
    result->it.GetFirst = GetFirst;
    result->Dict = Dict;
    result->it.Replace = ReplaceWithIterator;
    result->timestamp = Dict->timestamp;
    return 1;
}


static int deleteIterator(Iterator *it)
{
    struct ITERATOR *d = (struct ITERATOR *)it;
    DATA_TYPE *Dict;
    if (d ==NULL) {
        return NullPtrError("deleteIterator");
    }
    Dict = d->Dict;
    Dict->Allocator->free(it);
    return 1;
}

static Vector *CastToArray(const DATA_TYPE *Dict)
{
    size_t i;
    struct DATALIST *p;
    Vector *result;

    if (Dict == NULL) {
        NullPtrError("CastToArray");
        return NULL;
    }
    if (Dict->ElementSize == 0)
        return NULL;
    result = iVector.Create(Dict->ElementSize,Dict->count);

    for (i=0; i<Dict->size;i++) {
        for (p = Dict->buckets[i]; p; p = p->Next) {
            iVector.Add(result,(char *)p->Value);
        }
    }
    return result;
}

static int Save(const DATA_TYPE *Dict,FILE *stream, SaveFunction saveFn,void *arg)
{
    Vector *al;
    STRCOLLECTION *sc;
    int result = 1;

    if (Dict == NULL) {
        return NullPtrError("Save");
    }
    if (stream == NULL) {
        return BadArgError(Dict,"Save");
    }
    if (fwrite(&DictionaryGuid,sizeof(guid),1,stream) == 0) {
        return EOF;
    }
    al = CastToArray(Dict);
    sc = GetKeys(Dict);
    if ((iSTRCOLLECTION.Save(sc,stream,NULL,NULL) < 0) ||
        (iVector.Save(al,stream,saveFn,arg) < 0))
        result = EOF;
    iSTRCOLLECTION.Finalize(sc);
    iVector.Finalize(al);
    return result;
}
static DATA_TYPE *Copy(const DATA_TYPE *src)
{
    DATA_TYPE *result;
    size_t i;
    struct DATALIST *rvp;

    if (src == NULL) {
        NullPtrError("Copy");
        return NULL;
    }
    result = Create(src->ElementSize, src->count);
    if (result == NULL) {
        NoMemoryError(src,"Copy");
        return NULL;
    }
    result->Flags = (src->Flags&~CONTAINER_HAS_OBSERVER);
    result->hash = src->hash;
    result->RaiseError = src->RaiseError;
    for (i=0; i<src->size;i++) {
        rvp = src->buckets[i];
        while (rvp) {
            result->VTable->Add(result,rvp->Key,rvp->Value);
            rvp = rvp->Next;
        }
    }
    if (src->Flags & CONTAINER_HAS_OBSERVER)
        iObserver.Notify(src,CCL_COPY,result,NULL);
    return result;
}

static DATA_TYPE *Load(FILE *stream, ReadFunction readFn, void *arg)
{
    STRCOLLECTION *sc;
    Vector *al;
    DATA_TYPE *result;
    size_t i;
    guid Guid;

    if (stream == NULL) {
        NullPtrError("Load");
        return NULL;
    }
    if (fread(&Guid,sizeof(guid),1,stream) == 0) {
        iError.RaiseError("iDictionary.Load",CONTAINER_ERROR_FILE_READ);
        return NULL;
    }
    if (memcmp(&Guid,&DictionaryGuid,sizeof(guid))) {
        iError.RaiseError("iDictionary.Load",CONTAINER_ERROR_WRONGFILE);
        return NULL;
    }
    sc = iSTRCOLLECTION.Load(stream,NULL,NULL);
    if (sc == NULL)
        return NULL;
    al = iVector.Load(stream,readFn,arg);
    if (al == NULL) {
        iSTRCOLLECTION.Finalize(sc);
        return NULL;
    }
    result = Create(iVector.GetElementSize(al),iSTRCOLLECTION.Size(sc));
    for (i=0; i<iSTRCOLLECTION.Size(sc);i++) {
        CHARTYPE *key = (CHARTYPE *)iSTRCOLLECTION.GetElement(sc,i);
        void *data = iVector.GetElement(al,i);
        result->VTable->Add(result,key,data);
    }
    iSTRCOLLECTION.Finalize(sc);
    iVector.Finalize(al);
    return result;
}

static size_t GetElementSize(const DATA_TYPE *d)
{
    if (d == NULL) {
        NullPtrError("GetElementSize");
        return 0;
    }
    return d->ElementSize;
}

static const ContainerAllocator *GetAllocator(const DATA_TYPE *AL)
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
static DATA_TYPE *InitWithAllocator(DATA_TYPE *Dict,size_t elementsize,size_t hint,const ContainerAllocator *allocator)
{
    size_t i,allocSiz;
    static size_t primes[] = { 509, 509, 1021, 2053, 4093, 8191, 16381,
        32771, 65521, 131071, 262147, 524287, 1048573, 0 };
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
    Dict->VTable = &EXTERNAL_NAME;
    Dict->ElementSize = elementsize;
    Dict->Allocator = allocator;
    Dict->RaiseError = iError.RaiseError;
    return Dict;
}

static DATA_TYPE *Init(DATA_TYPE *dict,size_t elementsize,size_t hint)
{
    return InitWithAllocator(dict, elementsize, hint, CurrentAllocator);
}

static DATA_TYPE *CreateWithAllocator(size_t elementsize,size_t hint,const ContainerAllocator *allocator)
{
    DATA_TYPE *Dict,*result;

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
static DATA_TYPE *Create(size_t elementsize,size_t hint)
{
    return CreateWithAllocator(elementsize,hint,CurrentAllocator);
}

static DATA_TYPE *InitializeWith(size_t elementSize,size_t n, const CHARTYPE **Keys,const void *Values)
{
    DATA_TYPE *result = Create(elementSize,n);
    size_t i;
    const char *pValues = Values;

    if (result) {
        i=0;
        while (n-- > 0) {
            add_nd(result,Keys[i],pValues,0);
            i++;
            pValues += elementSize;
        }
    }
    return result;
}

static DestructorFunction SetDestructor(DATA_TYPE *cb,DestructorFunction fn)
{
    DestructorFunction oldfn;
    if (cb == NULL)
        return NULL;
    oldfn = cb->DestructorFn;
    if (fn)
        cb->DestructorFn = fn;
    return oldfn;
}

static HASHFUNCTION SetHashFunction(DATA_TYPE *d,HASHFUNCTION newFn)
{
    HASHFUNCTION old;
    if (d == NULL) {
        return hash;
    }
    if (newFn == NULL)
        return d->hash;
    old = d->hash;
    d->hash = newFn;
    return old;
}

static size_t SizeofIterator(const DATA_TYPE *b)
{
	return sizeof(struct ITERATOR);
}

static double GetLoadFactor(DATA_TYPE *d)
{
    return ((double)d->count)/d->size;
}

INTERFACE EXTERNAL_NAME  = {
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
    NewIterator,
    InitIterator,
    deleteIterator,
    SizeofIterator,
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
    InitializeWith,
    SetHashFunction,
    GetLoadFactor,
};
