/*
 Some algorithms for this code are from the Apache Runtime Library.
 */
#include "containers.h"
#include "ccl_internal.h"

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

/* Decode ULE128 string */

static int decode_ule128(FILE *stream, size_t *val)
{
        size_t i = 0;
        int c;

        val[0] = 0;
        do {
                c = fgetc(stream);
                if (c == EOF)
                        return EOF;
                val[0] += ((c & 0x7f) << (i * 7));
                i++;
        } while((0x80 & c) && (i < 2*sizeof(size_t)));
        return (int)i;
}

static int encode_ule128(FILE *stream,size_t val)
{
        int i=0;

        if (val == 0) {
                if (fputc(0, stream) == EOF)
                        return EOF;
                i=1;
        }
        else while (val) {
                size_t c = val&0x7f;
                val >>= 7;
                if (val)
                        c |= 0x80;
                if (fputc((int)c,stream) == EOF)
                        return EOF;
                i++;
        }
        return i;
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

static size_t GetElementSize(const HashTable *l)
{
    if (l) {
        return l->ElementSize;
    }
    iError.NullPtrError("GetElementSize");
    return 0;
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
			if (ht->DestructorFn)
				ht->DestructorFn(old);
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

static int Remove(HashTable *ht,const void *key,size_t klen)
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
			iError.RaiseError("iHashTable.Load",CONTAINER_ERROR_FILE_READ);
			free(keybuf);
			return NULL;
		}
		if (keybuflen < len) {
			void *tmp = realloc(keybuf,len);
			if (tmp == NULL) {
				iError.RaiseError("iHashTable.Load",CONTAINER_ERROR_NOMEMORY);
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
	d->Current = hi;
	return hi->this->val;
}

static void *GetCurrent(Iterator *it)
{
	struct HashTableIterator *d = (struct HashTableIterator *)it;
    return d->Current->this->val;
}

static int ReplaceWithIterator(Iterator *it, void *data,int direction) 
{
    struct HashTableIterator *li = (struct HashTableIterator *)it;
	int result;
	HashIndex current;
	
	if (it == NULL) {
		return NullPtrError("Replace");
	}
    if (li->timestamp != li->ht->timestamp) {
        li->ht->RaiseError("Replace",CONTAINER_ERROR_OBJECT_CHANGED);
        return CONTAINER_ERROR_OBJECT_CHANGED;
    }
	if (li->ht->Flags & CONTAINER_READONLY) {
		li->ht->RaiseError("Replace",CONTAINER_ERROR_READONLY);
		return CONTAINER_ERROR_READONLY;
	}
	if (li->ht->count == 0)
		return 0;
	current = *li->Current;
	GetNext(it);
	if (data == NULL)
		result = Remove(li->ht, current.this->key,current.this->klen);
	else {
		memcpy(current.this->val,data,li->ht->ElementSize);
		result = 1;
	}
	if (result >= 0) {
		li->timestamp = li->ht->timestamp;
	}
	return result;
}

static Iterator *NewIterator(HashTable *ht)
{
	struct HashTableIterator *result = 
	    ht->Allocator->malloc(sizeof(struct HashTableIterator));
	if (result == NULL)
		return NULL;
	result->it.GetNext = GetNext;
	result->it.GetPrevious = GetNext;
	result->it.GetFirst = GetFirst;
	result->it.GetCurrent = GetCurrent;
	result->it.Replace = ReplaceWithIterator;
	result->timestamp = ht->timestamp;
	result->Current = NULL;
	result->ht = ht;
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

static DestructorFunction SetDestructor(HashTable *cb,DestructorFunction fn)
{
	DestructorFunction oldfn;
	if (cb == NULL)
		return NULL;
	oldfn = cb->DestructorFn;
	if (fn)
		cb->DestructorFn = fn;
	return oldfn;
}


HashTableInterface iHashTable = {
Create,
Init,
Size,
	Sizeof,
GetFlags,
SetFlags,
GetElementSize,
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
NewIterator,
deleteIterator,
Save,
Load,
SetDestructor,
};

#if 0
/*
-------------------------------------------------------------------------------
lookup3.c, by Bob Jenkins, May 2006, Public Domain.

These are functions for producing 32-bit hashes for hash table lookup.
hashword(), hashlittle(), hashlittle2(), hashbig(), mix(), and final() 
are externally useful functions.  Routines to test the hash are included 
if SELF_TEST is defined.  You can use this free for any purpose.  It's in
the public domain.  It has no warranty.

You probably want to use hashlittle().  hashlittle() and hashbig()
hash byte arrays.  hashlittle() is is faster than hashbig() on
little-endian machines.  Intel and AMD are little-endian machines.
On second thought, you probably want hashlittle2(), which is identical to
hashlittle() except it returns two 32-bit hashes for the price of one.  
You could implement hashbig2() if you wanted but I haven't bothered here.

If you want to find a hash of, say, exactly 7 integers, do
  a = i1;  b = i2;  c = i3;
  mix(a,b,c);
  a += i4; b += i5; c += i6;
  mix(a,b,c);
  a += i7;
  final(a,b,c);
then use c as the hash value.  If you have a variable length array of
4-byte integers to hash, use hashword().  If you have a byte array (like
a character string), use hashlittle().  If you have several byte arrays, or
a mix of things, see the comments above hashlittle().  

Why is this so big?  I read 12 bytes at a time into 3 4-byte integers, 
then mix those integers.  This is fast (you can do a lot more thorough
mixing with 12*3 instructions on 3 integers than you can with 3 instructions
on 1 byte), but shoehorning those bytes into integers efficiently is messy.
-------------------------------------------------------------------------------
*/

#include <stdio.h>      /* defines printf for tests */
#include <time.h>       /* defines time_t for timings in the test */
#include <stdint.h>     /* defines uint32_t etc */
#include <sys/param.h>  /* attempt to define endianness */
#ifdef linux
# include <endian.h>    /* attempt to define endianness */
#endif

/*
 * My best guess at if you are big-endian or little-endian.  This may
 * need adjustment.
 */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
# define HASH_LITTLE_ENDIAN 1
# define HASH_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 1
#else
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 0
#endif

#define hashsize(n) ((uint32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose 
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/*
--------------------------------------------------------------------
 This works on all machines.  To be useful, it requires
 -- that the key be an array of uint32_t's, and
 -- that the length be the number of uint32_t's in the key

 The function hashword() is identical to hashlittle() on little-endian
 machines, and identical to hashbig() on big-endian machines,
 except that the length has to be measured in uint32_ts rather than in
 bytes.  hashlittle() is more complicated than hashword() only because
 hashlittle() has to dance around fitting the key bytes into registers.
--------------------------------------------------------------------
*/
static uint32_t hashword(
const uint32_t *k,                   /* the key, an array of uint32_t values */
size_t          length,               /* the length of the key, in uint32_ts */
uint32_t        initval)         /* the previous hash, or an arbitrary value */
{
  uint32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + (((uint32_t)length)<<2) + initval;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch(length)                     /* all the case statements fall through */
  { 
  case 3 : c+=k[2];
  case 2 : b+=k[1];
  case 1 : a+=k[0];
    final(a,b,c);
  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  return c;
}


/*
--------------------------------------------------------------------
hashword2() -- same as hashword(), but take two seeds and return two
32-bit values.  pc and pb must both be nonnull, and *pc and *pb must
both be initialized with seeds.  If you pass in (*pb)==0, the output 
(*pc) will be the same as the return value from hashword().
--------------------------------------------------------------------
*/
static void hashword2 (
const uint32_t *k,                   /* the key, an array of uint32_t values */
size_t          length,               /* the length of the key, in uint32_ts */
uint32_t       *pc,                      /* IN: seed OUT: primary hash value */
uint32_t       *pb)               /* IN: more seed OUT: secondary hash value */
{
  uint32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)(length<<2)) + *pc;
  c += *pb;

  /*------------------------------------------------- handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /*------------------------------------------- handle the last 3 uint32_t's */
  switch(length)                     /* all the case statements fall through */
  { 
  case 3 : c+=k[2];
  case 2 : b+=k[1];
  case 1 : a+=k[0];
    final(a,b,c);
  case 0:     /* case 0: nothing left to add */
    break;
  }
  /*------------------------------------------------------ report the result */
  *pc=c; *pb=b;
}


/*
-------------------------------------------------------------------------------
hashlittle() -- hash a variable-length key into a 32-bit value
  k       : the key (the unaligned variable-length array of bytes)
  length  : the length of the key, counting by bytes
  initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Two keys differing by one or two bits will have
totally different hash values.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (uint8_t **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);

By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
-------------------------------------------------------------------------------
*/

static uint32_t hashlittle( const void *key, size_t length, uint32_t initval)
{
  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /* 
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }

#else /* make valgrind happy */
    {
    const uint8_t *k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
    case 9 : c+=k8[8];                   /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
    case 5 : b+=k8[4];                   /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
    case 1 : a+=k8[0]; break;
    case 0 : return c;
    }
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : return c;                     /* zero length requires no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}


/*
 * hashlittle2: return 2 32-bit hash values
 *
 * This is identical to hashlittle(), except it returns two 32-bit hash
 * values instead of just one.  This is good enough for hash table
 * lookup with 2^^64 buckets, or if you want a second hash if you're not
 * happy with the first, or if you want a probably-unique 64-bit ID for
 * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 */
static void hashlittle2( 
  const void *key,       /* the key to hash */
  size_t      length,    /* length of the key */
  uint32_t   *pc,        /* IN: primary initval, OUT: primary hash */
  uint32_t   *pb)        /* IN: secondary initval, OUT: secondary hash */
{
  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + *pc;
  c += *pb;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /* 
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#else /* make valgrind happy */
    {
    const uint8_t *k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
    case 9 : c+=k8[8];                   /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
    case 5 : b+=k8[4];                   /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
    case 1 : a+=k8[0]; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }
  }

  final(a,b,c);
  *pc=c; *pb=b;
}



/*
 * hashbig():
 * This is the same as hashword() on big-endian machines.  It is different
 * from hashlittle() on all machines.  hashbig() takes advantage of
 * big-endian byte ordering. 
 */
static uint32_t hashbig( const void *key, size_t length, uint32_t initval)
{
  uint32_t a,b,c;
  union { const void *ptr; size_t i; } u; /* to cast key to (size_t) happily */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_BIG_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /* 
     * "k[2]<<8" actually reads beyond the end of the string, but
     * then shifts out the part it's not allowed to read.  Because the
     * string is aligned, the illegal read is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff00; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff0000; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff000000; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff00; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff0000; a+=k[0]; break;
    case 5 : b+=k[1]&0xff000000; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff00; break;
    case 2 : a+=k[0]&0xffff0000; break;
    case 1 : a+=k[0]&0xff000000; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }

#else  /* make valgrind happy */
    {
    const uit8_t *k8 = (const uint8_t *)k;
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<8;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<16;  /* fall through */
    case 9 : c+=((uint32_t)k8[8])<<24;  /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<8;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<16;  /* fall through */
    case 5 : b+=((uint32_t)k8[4])<<24;  /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<8;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<16;  /* fall through */
    case 1 : a+=((uint32_t)k8[0])<<24; break;
    case 0 : return c;
    }
    }

#endif /* !VALGRIND */

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += ((uint32_t)k[0])<<24;
      a += ((uint32_t)k[1])<<16;
      a += ((uint32_t)k[2])<<8;
      a += ((uint32_t)k[3]);
      b += ((uint32_t)k[4])<<24;
      b += ((uint32_t)k[5])<<16;
      b += ((uint32_t)k[6])<<8;
      b += ((uint32_t)k[7]);
      c += ((uint32_t)k[8])<<24;
      c += ((uint32_t)k[9])<<16;
      c += ((uint32_t)k[10])<<8;
      c += ((uint32_t)k[11]);
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=k[11];
    case 11: c+=((uint32_t)k[10])<<8;
    case 10: c+=((uint32_t)k[9])<<16;
    case 9 : c+=((uint32_t)k[8])<<24;
    case 8 : b+=k[7];
    case 7 : b+=((uint32_t)k[6])<<8;
    case 6 : b+=((uint32_t)k[5])<<16;
    case 5 : b+=((uint32_t)k[4])<<24;
    case 4 : a+=k[3];
    case 3 : a+=((uint32_t)k[2])<<8;
    case 2 : a+=((uint32_t)k[1])<<16;
    case 1 : a+=((uint32_t)k[0])<<24;
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}
#endif
