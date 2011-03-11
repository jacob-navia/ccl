/*
 Some algorithms for this code are from the Apache Runtime Library.
 */
#include "containers.h"
#include "ccl_internal.h"

typedef struct _HashEntry {
    struct _HashEntry *next;
    unsigned int       hash;
    const void        *key;
    size_t             klen;
    char              val[1];
} HashEntry  ;

/*
 * Data structure for iterating through a hash table.
 *
 * We keep a pointer to the next hash entry here to allow the current
 * hash entry to be freed or otherwise mangled between calls to
 * hash_next().
 */
typedef struct _HashIndex {
    HashTable     *ht;
    HashEntry     *this, *next;
    unsigned int   index;
} HashIndex;

/*
 * The size of the array is always a power of two. We use the maximum
 * index rather than the size so that we can use bitwise-AND for
 * modular arithmetic.
 * The count of hash entries may be greater depending on the chosen
 * collision rate.
 */
struct _HashTable {
	HashTableInterface *VTable;
    Pool          *pool;
    HashEntry   **array;
    HashIndex     iterator;  /* For hash_first(NULL, ...) */
    unsigned int   count, max;
    HashFunction   Hash;
    HashEntry     *free;  /* List of recycled entries */
	unsigned       Flags;
	ErrorFunction  RaiseError;
	unsigned       timestamp;
	size_t         ElementSize;
	ContainerMemoryManager *Allocator;
};

static const guid HashTableGuid = {0x3a3d3aab, 0xb14a, 0x4249,
{0x98,0x2,0xbd,0xdc,0xa5,0x62,0x59,0x75}
};


#define INITIAL_MAX 15 /* tunable == 2^n - 1 */
static unsigned int DefaultHashFunction(const char *char_key, size_t *klen);
static HashTable * Merge(Pool *p, const HashTable *overlay, const HashTable *base,
                  void * (*merger)(Pool *p, const void *key, size_t klen,
                                   const void *h1_val, const void *h2_val,
                                   const void *data),
                  const void *data);
typedef int (ApplyCallback)(void *rec, const void *key,size_t klen,const void *value);
static HashIndex *first(HashIndex *hi);
static HashIndex *next(HashIndex *hi);
/*
 * Hash creation functions.
 */
static int NullPtrError(const char *fnName)
{
	char buf[256];
	sprintf(buf,"iHashTable.%s",fnName);
	iError.RaiseError(buf,CONTAINER_ERROR_BADARG);
	return CONTAINER_ERROR_BADARG;
}


static HashEntry **alloc_array(HashTable *ht, size_t max)
{
   return iPool.Calloc(ht->pool, (max+1),(sizeof(*ht->array) + ht->ElementSize ));
}

static HashTable * Create(size_t ElementSize)
{
    HashTable *ht;
    Pool *pool = iPool.Create(NULL);
    ht = iPool.Calloc(pool, 1, sizeof(HashTable));
    ht->pool = pool;
    ht->max = INITIAL_MAX;
    ht->array = alloc_array(ht, ht->max);
    ht->Hash = DefaultHashFunction;
	ht->VTable = &iHashTable;
	ht->ElementSize = ElementSize;
    return ht;
}

static HashTable *Init(HashTable *ht,size_t ElementSize)
{
	iError.RaiseError("iHashTable.Init",CONTAINER_ERROR_NOTIMPLEMENTED);
	return NULL;
}
static HashFunction SetHashFunction(HashTable *ht, HashFunction Hash)
{
    HashFunction old = ht->Hash;
    if (Hash)
        ht->Hash = Hash;
    return old;
}

/*
 * Resizing a hash table
 */

static int Resize(HashTable *ht,size_t newsize)
{
    HashIndex hi,*hiptr;
    HashEntry **new_array;
    size_t new_max;
	char *p;

	if (ht == NULL)
		return CONTAINER_ERROR_BADARG;
	if (newsize == 0)
		new_max = ht->max * 2 + 1;
	else new_max = newsize;
    new_array = alloc_array(ht, new_max);
    for (hiptr = first(&hi); hiptr; hiptr = next(hiptr)) {
        size_t i = hiptr->this->hash % new_max;
		p = (char *)new_array;
		p += i * (ht->ElementSize+sizeof(HashEntry));
		hiptr->this->next = (HashEntry *)(p+sizeof(HashEntry)+ht->ElementSize);
        memcpy(p, hiptr->this,sizeof(HashEntry)+ht->ElementSize);
    }
    ht->array = new_array;
    ht->max = (unsigned)new_max;
	return 1;
}

static unsigned int DefaultHashFunction(const char *char_key, size_t *klen)
{
    unsigned int hash = 0;
    const unsigned char *key = (const unsigned char *)char_key;
    const unsigned char *p;
    size_t i;
    
    /*
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
     
    if (*klen == (size_t)-1) {
        for (p = key; *p; p++) {
            hash = hash * 33 + *p;
        }
        *klen = p - key;
    }
    else {
        for (p = key, i = *klen; i; i--, p++) {
            hash = hash * 33 + *p;
        }
    }

    return hash;
}


/*
 * This is where we keep the details of the hash function and control
 * the maximum collision rate.
 *
 * If val is non-NULL it creates and initializes a new hash entry if
 * there isn't already one there; it returns an updatable pointer so
 * that hash entries can be removed.
 */

static HashEntry **find_entry(HashTable *ht,const void *key,size_t klen,const void *val)
{
    HashEntry **hep, *he;
    unsigned int hash;

    hash = ht->Hash(key, &klen);

    /* scan linked list */
    for (hep = &ht->array[hash & ht->max], he = *hep;
         he; hep = &he->next, he = *hep) {
        if (he->hash == hash
            && he->klen == klen
            && memcmp(he->key, key, klen) == 0)
            break;
    }
    if (he || !val)
        return hep;

    /* add a new entry for non-NULL values */
    if ((he = ht->free) != NULL)
        ht->free = he->next;
    else
        he = iPool.Alloc(ht->pool, sizeof(*he));
    he->next = NULL;
    he->hash = hash;
    he->key  = key;
    he->klen = klen;
    memcpy(he->val,val,ht->ElementSize);
    *hep = he;
    ht->count++;
    return hep;
}

static int Replace(HashTable *ht,const void *key,size_t klen,const void *val)
{
    HashEntry **hep, *he;
    unsigned int hash;
	
	if (ht == NULL ||val == NULL || key == NULL || klen == 0) {
		iError.RaiseError("iHashTable.Replace",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
    hash = ht->Hash(key, &klen);
	
    /* scan linked list */
    for (hep = &ht->array[hash & ht->max], he = *hep;
         he; hep = &he->next, he = *hep) {
        if (he->hash == hash
            && he->klen == klen
            && memcmp(he->key, key, klen) == 0) {
            memcpy(he->val,val,ht->ElementSize);
			return 1;
		}
    }
    return 0;
}

static int Add(HashTable *ht,const void *key, size_t klen, const void *val)
{
	size_t oldCount;
	if (ht == NULL || key == NULL || klen == 0) {
		iError.RaiseError("iHashTable.Add",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	oldCount = ht->count;
	find_entry(ht,key,klen,val); 
	return oldCount != ht->count;
}

static void *GetElement(const HashTable *ht,const void *key, size_t klen)
{
	HashEntry **v = find_entry((HashTable *)ht,key, klen, NULL);
	if (v)
		return (void *)((*v)->val);
	return NULL;
}
static HashTable *Copy( const HashTable *orig,Pool *pool)
{
    HashTable *ht;
    HashEntry *new_vals;
    unsigned int i, j;

	if (orig == NULL) {
		iError.RaiseError("iHashTable.Copy",CONTAINER_ERROR_BADARG);
		return NULL;
	}
	if (pool == NULL)
		pool = orig->pool;
    ht = iPool.Alloc(pool, sizeof(HashTable) +
                    sizeof(*ht->array) * (orig->max + 1) +
                    (sizeof(HashEntry)+orig->ElementSize) * orig->count);
    ht->pool = pool;
    ht->free = NULL;
    ht->count = orig->count;
    ht->max = orig->max;
    ht->Hash = orig->Hash;
    ht->array = (HashEntry **)((char *)ht + sizeof(HashTable));

    new_vals = (HashEntry *)((char *)(ht) + sizeof(HashTable) +
                                    sizeof(*ht->array) * (orig->max + 1));
    j = 0;
    for (i = 0; i <= ht->max; i++) {
        HashEntry **new_entry = &(ht->array[i]);
        HashEntry *orig_entry = orig->array[i];
        while (orig_entry) {
            *new_entry = &new_vals[j++];
            (*new_entry)->hash = orig_entry->hash;
            (*new_entry)->key = orig_entry->key;
            (*new_entry)->klen = orig_entry->klen;
            memcpy((*new_entry)->val , orig_entry->val,ht->ElementSize);
            new_entry = &((*new_entry)->next);
            orig_entry = orig_entry->next;
        }
        *new_entry = NULL;
    }
    return ht;
}

static HashEntry **HashSet(HashTable *ht, const void *key, size_t klen, const void *val)
{
    HashEntry **hep = find_entry(ht, key, klen, val);
    if (*hep) {
        if (!val) {
            /* delete entry */
            HashEntry *old = *hep;
            *hep = (*hep)->next;
            old->next = ht->free;
            ht->free = old;
            --ht->count;
        }
        else {
            /* replace entry */
            memcpy((*hep)->val , (void *)val,sizeof(HashEntry)+ht->ElementSize);
            /* check that the collision rate isn't too high */
            if (ht->count > ht->max) {
                Resize(ht,0);
            }
        }
    }
    /* else key not present and val==NULL */
	return hep;
}

static int Remove(HashTable *ht,void *key,size_t klen)
{
	HashEntry **hep = HashSet(ht,key,klen,NULL);
	if (hep)
		return 1;
	return 0;
}

static int Clear(HashTable *ht)
{
    HashIndex HashIdx,*hi;
    for (hi = first(&HashIdx); hi; hi = next(hi))
        HashSet(ht, hi->this->key, hi->this->klen, NULL);
	return 1;
}

static int Finalize(HashTable *ht)
{
	Clear(ht);
	iPool.Destroy(ht->pool);
	return 1;
}

static HashTable* Overlay(Pool *p, const HashTable *overlay, const HashTable *base)
{
    return Merge(p, overlay, base, NULL, NULL);
}

static HashTable * Merge(Pool *p, const HashTable *overlay, const HashTable *base,
                                         void * (*merger)(Pool *p,
                                                     const void *key,
                                                     size_t klen,
                                                     const void *h1_val,
                                                     const void *h2_val,
                                                     const void *data),
                                         const void *data)
{
    HashTable *res;
    HashEntry *new_vals = NULL;
    HashEntry *iter;
    HashEntry *ent;
    unsigned int i,j,k;
	void *pvoid;

	if (p == NULL || overlay == NULL || base == NULL) {
		iError.RaiseError("iHashTable.Merge",CONTAINER_ERROR_BADARG);
		return NULL;
	}
    res = iPool.Alloc(p, sizeof(HashTable));
    res->pool = p;
    res->free = NULL;
    res->Hash = base->Hash;
    res->count = base->count;
    res->max = (overlay->max > base->max) ? overlay->max : base->max;
    if (base->count + overlay->count > res->max) {
        res->max = res->max * 2 + 1;
    }
    res->array = alloc_array(res, res->max);
    if (base->count + overlay->count) {
        new_vals = iPool.Alloc(p, (sizeof(HashEntry)+base->ElementSize) *
                              (base->count + overlay->count));
    }
	if (new_vals == NULL) {
		return NULL;
	}
    j = 0;
    for (k = 0; k <= base->max; k++) {
        for (iter = base->array[k]; iter; iter = iter->next) {
            i = iter->hash & res->max;
            new_vals[j].klen = iter->klen;
            new_vals[j].key = iter->key;
            memcpy(new_vals[j].val , iter->val,base->ElementSize);
            new_vals[j].hash = iter->hash;
            new_vals[j].next = res->array[i];
            res->array[i] = &new_vals[j];
            j++;
        }
    }

    for (k = 0; k <= overlay->max; k++) {
        for (iter = overlay->array[k]; iter; iter = iter->next) {
            i = iter->hash & res->max;
            for (ent = res->array[i]; ent; ent = ent->next) {
                if ((ent->klen == iter->klen) &&
                    (memcmp(ent->key, iter->key, iter->klen) == 0)) {
                    if (merger) {
                        pvoid = (*merger)(p, iter->key, iter->klen,
                                             iter->val, ent->val, data);						
                    }
                    else {
                        pvoid = iter->val;
                    }
					memcpy(ent->val,pvoid,base->ElementSize);
                    break;
                }
            }
            if (!ent) {
                new_vals[j].klen = iter->klen;
                new_vals[j].key = iter->key;
                memcpy(new_vals[j].val , iter->val,base->ElementSize);
                new_vals[j].hash = iter->hash;
                new_vals[j].next = res->array[i];
                res->array[i] = &new_vals[j];
                res->count++;
                j++;
            }
        }
    }
    return res;
}

/* This is basically the following...
 * for every element in hash table {
 *    comp elemeny.key, element.value
 * }
 *
 * Like with apr_table_do, the comp callback is called for each and every
 * element of the hash table.
 */
static int Search(HashTable *ht,ApplyCallback *comp, void *rec)
{
    HashIndex  hix;
    HashIndex *hi;
    int rv, dorv  = 1;

    hix.ht    = (HashTable *)ht;
    hix.index = 0;
    hix.this  = NULL;
    hix.next  = NULL;

	hi = next(&hix);
    if (hi) {
        /* Scan the entire table */
        do {
            rv = (*comp)(rec, hi->this->key, hi->this->klen, hi->this->val);
        } while (rv && (hi = next(hi)));

        if (rv == 0) {
            dorv = 0;
        }
    }
    return dorv;
}
static int Apply(HashTable *ht,int (*Applyfn)(void *Key,size_t klen,void *data,void *arg), void *arg)
{
    HashIndex  hix;
    HashIndex *hi;
    int rv, dorv  = 1;
	
    hix.ht    = (HashTable *)ht;
    hix.index = 0;
    hix.this  = NULL;
    hix.next  = NULL;
	
	hi = next(&hix);
    if (hi) {
        /* Scan the entire table */
        do {
            rv = (*Applyfn)((void *)hi->this->key, hi->this->klen,(void *) hi->this->val,arg);
        } while (rv && (hi = next(hi)));
		
        if (rv == 0) {
            dorv = 0;
        }
    }
    return dorv;
}
static ErrorFunction SetErrorFunction(HashTable *ht,ErrorFunction fn)
{
	ErrorFunction old;
	old = ht->RaiseError;
	ht->RaiseError = (fn) ? fn : iError.EmptyErrorFunction;
	return old;
}

static size_t Size(const HashTable *AL)
{
	return AL->count;
}
static size_t Sizeof(const HashTable *HT)
{
	if (HT == NULL)
		return sizeof(HashTable);
	return sizeof(HashTable) + HT->count * (sizeof(HashEntry)+HT->ElementSize);
}
static unsigned GetFlags(const HashTable *AL)
{
	return AL->Flags;
}
static unsigned SetFlags(HashTable *AL,unsigned newval)
{
	int oldval = AL->Flags;
	AL->Flags = newval;
	return oldval;
}
static size_t DefaultSaveFunction(const void *element,void *arg, FILE *Outfile)
{
	size_t *pLength = arg;
	size_t len = *pLength;
	
	return fwrite(element,1,len,Outfile);
}

static size_t DefaultLoadFunction(void *element,void *arg, FILE *Infile)
{
	size_t len = *(size_t *)arg;
	
	return fread(element,1,len,Infile);
}

static int Save(HashTable *HT,FILE *stream, SaveFunction saveFn,void *arg)
{
    HashIndex  hix;
    HashIndex *hi;
    int rv;
	int retval=1;
	
	if (HT == NULL || stream == NULL) {
		return NullPtrError("Save");
	}
	if (saveFn == NULL) {
		saveFn = DefaultSaveFunction;
		arg = &HT->ElementSize;
	}
	if (fwrite(&HashTableGuid,sizeof(guid),1,stream) <= 0)
		return EOF;
	if (fwrite(HT,1,sizeof(HashTable),stream) <= 0)
		return EOF;
	
    hix.ht    = HT;
    hix.index = 0;
    hix.this  = NULL;
    hix.next  = NULL;
	
	hi = next(&hix);
    if (hi) {
        /* Scan the entire table */
        do {
			rv = encode_ule128(stream, hi->this->klen);
			if (rv > 0)
				rv = (int)fwrite(hi->this->key,1,hi->this->klen,stream);
			if (rv > 0)
				rv = (int)saveFn(hi->this->val,arg,stream);
        } while (rv > 0 && (hi = next(hi)));
		
        if (rv <= 0) {
            retval = (int)rv;
        }
    }
    return retval;	
}

static HashTable *Load(FILE *stream, ReadFunction readFn,void *arg)
{
	size_t i,len,keybuflen;
	HashTable *result,HT;
	char *keybuf,*valbuf;
	guid Guid;
	
	if (readFn == NULL) {
		readFn = DefaultLoadFunction;
		arg = &HT.ElementSize;
	}

	if (fread(&Guid,sizeof(guid),1,stream) <= 0) {
		iError.RaiseError("iHashTable.Load",CONTAINER_ERROR_FILE_READ);
		return NULL;
	}
	if (memcmp(&Guid,&HashTableGuid,sizeof(guid))) {
		iError.RaiseError("iHashTable.Load",CONTAINER_ERROR_WRONGFILE);
		return NULL;
	}
	if (fread(&HT,1,sizeof(HashTable),stream) <= 0) {
		iError.RaiseError("HashTable.Load",CONTAINER_ERROR_FILE_READ);
		return NULL;
	}
	result = iHashTable.Create(HT.ElementSize);
	if (result == NULL)
		return NULL;
	keybuflen = 4096;
	keybuf = malloc(keybuflen);
	valbuf = malloc(HT.ElementSize);
	result->Flags = HT.Flags;
	for (i=0; i< HT.count; i++) {
		if (decode_ule128(stream, &len) <= 0) {
		err:
			iError.RaiseError("StringCollection.Load",CONTAINER_ERROR_FILE_READ);
			free(keybuf);
			return NULL;
		}
		if (keybuflen < len) {
			void *tmp = realloc(keybuf,len);
			if (tmp == NULL) {
				iError.RaiseError("HashTable.Load",CONTAINER_ERROR_NOMEMORY);
				free(keybuf);
				return NULL;
			}
			keybuf = tmp;
			keybuflen = len;
		}
		if (fread(keybuf,1,len,stream) <= 0) {
			goto err;
		}
		if (readFn(valbuf,arg,stream) <= 0) {
			goto err;
		}
		iHashTable.Add(result,keybuf,len,valbuf);
	}
	free(keybuf);
	return result;
}

/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */

struct HashTableIterator {
	Iterator it;
	HashIndex hi;
	size_t timestamp;
	unsigned long Flags;
	void *Current;
};

static HashIndex * next(HashIndex *hi)
{
    hi->this = hi->next;
    while (!hi->this) {
        if (hi->index > hi->ht->max)
            return NULL;
		
        hi->this = hi->ht->array[hi->index++];
    }
    hi->next = hi->this->next;
    return hi;
}

static void *GetNext(Iterator *it)
{
	struct HashTableIterator *d = (struct HashTableIterator *)it;
	HashIndex *hi = next(&d->hi);
	d->Current = hi;
	if (hi) {
		return (void *)hi->this->val;
	}
	return NULL;
}

static HashIndex *first(HashIndex *hi)
{
	hi->index = 0;
    hi->this = NULL;
    hi->next = NULL;
	return next(hi);
}

static void *GetFirst(Iterator *it)
{
    HashIndex *hi;
	struct HashTableIterator *d = (struct HashTableIterator *)it;

    hi = first(&d->hi);
	d->Current = hi;
	if (hi == NULL)
		return NULL;
	return hi->this->val;
}

static void *GetCurrent(Iterator *it)
{
	struct HashTableIterator *d = (struct HashTableIterator *)it;
    return d->Current;
}
static Iterator *newIterator(HashTable *ht)
{
	struct HashTableIterator *result = MALLOC(ht,sizeof(struct HashTableIterator));
	if (result == NULL)
		return NULL;
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetNext;
	result->it.GetFirst = GetFirst;
	result->it.GetCurrent = GetCurrent;
	result->timestamp = ht->timestamp;
	result->Current = NULL;
	return &result->it;
}

static int deleteIterator(Iterator *it)
{
#if 0
	struct HashTableIterator *d = (struct HashTableIterator *)it;
	HashTable *ht = d->hi.ht;
	//FREE(ht,it);
#endif
	return 1;
}

HashTableInterface iHashTable = {
Create,
Init,
Size,
	Sizeof,
GetFlags,
SetFlags,
Add,
Clear,
GetElement,
Search,
Remove,
Finalize,
Apply,
SetErrorFunction,
Resize,
Replace,
Copy,
SetHashFunction,
Overlay,
Merge,
newIterator,
deleteIterator,
Save,
Load,

};