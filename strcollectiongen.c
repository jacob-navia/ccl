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

static int NullPtrError(const char *fnName)
{
    char buf[512];

    snprintf(buf,sizeof(buf),"istrCollection.%s",fnName);
    iError.RaiseError(buf,CONTAINER_ERROR_BADARG);
    return CONTAINER_ERROR_BADARG;
}
static int doerrorCall(ErrorFunction err,const char *fnName,int code)
{
    char buf[256];

    snprintf(buf,sizeof(buf),"strCollection.%s",fnName);
    err(buf,code);
    return code;
}
static int ReadOnlyError(const ElementType *SC,const char *fnName)
{
    return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_READONLY);
}

static int BadArgError(const ElementType *SC,const char *fnName)
{
    return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_BADARG);
}


static int IndexError(const ElementType *SC,size_t idx,const char *fnName)
{
    char buf[256];

    snprintf(buf,sizeof(buf),"strCollection.%s",fnName);
    SC->RaiseError(buf,CONTAINER_ERROR_INDEX,SC,idx);
    return CONTAINER_ERROR_INDEX;
}

static int NoMemoryError(const ElementType *SC,const char *fnName)
{
    return doerrorCall(SC->RaiseError,fnName,CONTAINER_ERROR_NOMEMORY);
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
static CHAR_TYPE *DuplicateString(const ElementType *SC,const CHAR_TYPE *str,const char *fnname)
{
    CHAR_TYPE *result;
    if (str == NULL)      return NULL;
    result = SC->Allocator->malloc(sizeof(CHAR_TYPE)*(1+STRLEN(str)));
    if (result == NULL) {
        NoMemoryError(SC,fnname);
        return NULL;
    }
    STRCPY(result,str);
    return result;
}

#define SC_IGNORECASE   (CONTAINER_READONLY << 1)
#define CHUNKSIZE 20

static size_t GetCount(const ElementType *SC)
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
static unsigned GetFlags(const ElementType *SC)
{
    if (SC == NULL) {
        NullPtrError("GetFlags");
        return 0;
    }
    return SC->Flags;
}

static const ContainerAllocator *GetAllocator(const ElementType *AL)
{
    if (AL == NULL) {
        return NULL;
    }
    return AL->Allocator;
}

static int Mismatch(const ElementType *a1,const ElementType *a2,size_t *mismatch)
{
    size_t siz,i;
    CompareInfo ci;
    CHAR_TYPE **p1,**p2;

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
    ci.ContainerLeft = (ElementType *)a1;
    ci.ContainerRight = (ElementType *)a2;
    ci.ExtraArgs = NULL;
    for (i=0;i<siz;i++) {
        if (a1->strcompare((const void **)p1,(const void **)p2,&ci) != 0) {
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
static unsigned SetFlags(ElementType *SC,unsigned newval)
{
    unsigned oldval;
    
    if (SC == NULL) {
        NullPtrError("SetFlags");
        return 0;
    }
    oldval = SC->Flags;
    SC->Flags = newval;
    SC->timestamp++;
    return oldval;
}
static int ResizeTo(ElementType *SC,size_t newcapacity)
{
    CHAR_TYPE **oldcontents;

    if (SC == NULL) {
        return NullPtrError("ResizeTo");
    }
    oldcontents = SC->contents;
    SC->contents = SC->Allocator->malloc(newcapacity*sizeof(CHAR_TYPE *));
    if (SC->contents == NULL) {
        SC->contents = oldcontents;
        return NoMemoryError(SC,"ResizeTo");
    }
    memset(SC->contents,0,sizeof(CHAR_TYPE *)*newcapacity);
    memcpy(SC->contents,oldcontents,SC->count*sizeof(CHAR_TYPE *));
    SC->capacity = newcapacity;
    if (SC->DestructorFn)
        SC->DestructorFn(oldcontents);
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
static int Resize(ElementType *SC)
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
static int Add(ElementType *SC,const CHAR_TYPE *newval)
{
    if (SC == NULL) {
        return NullPtrError("Add");
    }

    if (SC->Flags & CONTAINER_READONLY)
        return ReadOnlyError(SC,"Add");
    if ((SC->count+1) >= SC->capacity) {
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

static int AddRange(ElementType *SC,size_t n, const CHAR_TYPE **data)
{
    size_t newcapacity;
    CHAR_TYPE **p;

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
        CHAR_TYPE **newcontents;
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

static int Append(ElementType *SC1, ElementType *SC2)
{
    if (SC1 == NULL || SC2 == NULL) {
        return NullPtrError("Append");
    }
    if (SC1->Flags & CONTAINER_READONLY) {
        return ReadOnlyError(SC1,"Append");
    }
    return AddRange(SC1,SC2->count,(const CHAR_TYPE **)SC2->contents);
}

static int Clear(ElementType *SC)
{
    size_t i;

    if (SC == NULL) {
        return NullPtrError("Clear");
    }
    if (SC->Flags & CONTAINER_READONLY)
        return ReadOnlyError(SC,"Clear");
    for (i=0; i<SC->count;i++) {
        if (SC->DestructorFn)
            SC->DestructorFn(SC->contents[i]);
        SC->Allocator->free(SC->contents[i]);
        SC->contents[i] = NULL;
    }
    SC->count = 0;
    SC->timestamp=0;
    SC->Flags=0;
    return 1;
}


static int Contains(const ElementType *SC,const CHAR_TYPE *str)
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
        if (c == SC->contents[i][0] && !SC->strcompare((const void **)&SC->contents[i],
            (const void **)&str,NULL))
            return 1;
    }
    return 0;
}

static CHAR_TYPE **CopyTo(const ElementType *SC)
{
    CHAR_TYPE **result;
    size_t i;

    if (SC == NULL) {
        NullPtrError("CopyTo");
        return NULL;
    }

    result = SC->Allocator->malloc((1+SC->count)*sizeof(CHAR_TYPE *));
    if (result == NULL) {
        NoMemoryError(SC,"CopyTo");
        return NULL;
    }
    for (i=0; i<SC->count;i++) {
        result[i] = DuplicateString(SC,SC->contents[i],"CopyTo");
        if (result[i] == NULL) {
            while (i > 0) {
                --i;
                SC->Allocator->free(result[i]);
            }
            SC->Allocator->free(result);
            return NULL;
        }
    }
    result[i] = NULL;
    return result;
}

static int IndexOf(const ElementType *SC,const CHAR_TYPE *str,size_t *result)
{
    size_t i;

    if (SC == NULL) {
        return NullPtrError("IndexOf");
    }
    if (str == NULL)
        return BadArgError(SC,"IndexOf");

    for (i=0; i<SC->count;i++) {
        if (!SC->strcompare((const void **)&SC->contents[i],
            (const void **)&str,SC->StringCompareContext)) {
            *result = i;
            return 1;
        }
    }
    return CONTAINER_ERROR_NOTFOUND;
}
static int Finalize(ElementType *SC)
{
    size_t i;

    if (SC == NULL) {
        return CONTAINER_ERROR_BADARG;
    }
    if (SC->Flags & CONTAINER_READONLY) {
        return ReadOnlyError(SC,"Finalize");
    }

    for (i=0; i<SC->count;i++) {
        if (SC->DestructorFn)
            SC->DestructorFn(SC->contents[i]);
        SC->Allocator->free(SC->contents[i]);
    }
    SC->Allocator->free(SC->contents);
    if (SC->VTable != &INTERFACE_OBJECT)
        SC->Allocator->free(SC->VTable);
    SC->Allocator->free(SC);
    return 1;
}

static CHAR_TYPE *GetElement(const ElementType *SC,size_t idx)
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
        IndexError(SC,idx,"GetElement");
        return NULL;
    }
    return SC->contents[idx];
}

static ElementType *IndexIn(const ElementType *SC,const Vector *AL)
{
    ElementType *result = NULL;
    size_t i,top,idx;
    const CHAR_TYPE *p;
    int r;

    if (SC == NULL || AL == NULL) {
        NullPtrError("IndexIn");
        return NULL;
    }
    if (iVector.GetElementSize(AL) != sizeof(size_t)) {
        SC->RaiseError("istrCollection.IndexIn",CONTAINER_ERROR_INCOMPATIBLE);
        return NULL;
    }
    top = iVector.Size(AL);
    result = iElementType.Create(top);
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

static int InsertAt(ElementType *SC,size_t idx,const CHAR_TYPE *newval)
{
    CHAR_TYPE *p;
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
        return IndexError(SC,idx,"InsertAt");
    }
    if ((SC->count+1) >= SC->capacity) {
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
            memmove(SC->contents+1,SC->contents,SC->count*sizeof(CHAR_TYPE *));
        SC->contents[0] = p;
    }
    else if (idx == SC->count) {
        SC->contents[idx] = p;
    }
    else if (idx < SC->count) {
        memmove(SC->contents+idx+1,SC->contents+idx,(SC->count-idx+1)*sizeof(CHAR_TYPE *));
        SC->contents[idx] = p;
    }
    SC->timestamp++;
    ++SC->count;
    return 1;
}

static int InsertIn(ElementType *source, size_t idx, ElementType *newData)
{
    size_t newCount,i,j,siz;
    CHAR_TYPE **p,**oldcontents;

    if (source == NULL || newData == NULL) {
        return NullPtrError("InsertIn");
    }
    if (source->Flags & CONTAINER_READONLY)
        return ReadOnlyError(source,"InsertIn");
    if (idx > source->count) {
        return IndexError(source,idx,"InsertIn");
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
    siz = source->capacity*sizeof(CHAR_TYPE *);
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

static int Insert(ElementType *SC,CHAR_TYPE *newval)
{
    return InsertAt(SC,0,newval);
}

static int RemoveAt(ElementType *SC,size_t idx)
{
    if (SC == NULL) {
        return NullPtrError("RemoveAt");
    }
    if (idx >= SC->count )
        return IndexError(SC,idx,"RemoveAt");
    if (SC->Flags & CONTAINER_READONLY)
        return ReadOnlyError(SC,"RemoveAt");
    /* Test for remove of an empty collection */
    if (SC->count == 0)
        return 0;
    if (SC->DestructorFn)
        SC->DestructorFn(SC->contents[idx]);
    SC->Allocator->free(SC->contents[idx]);
    if (idx < (SC->count-1)) {
        memmove(SC->contents+idx,SC->contents+idx+1,(SC->count-idx)*sizeof(char *));
    }
    SC->contents[SC->count-1]=NULL;
    SC->timestamp++;
    --SC->count;
    return 1;
}

static int RemoveRange(ElementType *SC,size_t start, size_t end)
{
    size_t i;
    if (SC == NULL)
        return NullPtrError("RemoveRange");
    if (SC->count == 0)
        return 0;
    if (end > SC->count)
        end = SC->count;
    if (start == end) return 0;
    if (start >= SC->count)
        return IndexError(SC,start,"RemoveRange");
    if (SC->DestructorFn) {
        for (i=start; i<end; i++) {
            SC->DestructorFn(SC->contents[i]);
            SC->Allocator->free(SC->contents[i]);
        }
    }
    else {
        for (i=start; i<end; i++) {
            SC->Allocator->free(SC->contents[i]);
        }
    }
    if (end < SC->count)
    memmove(SC->contents+start,
        SC->contents+end,
        (SC->count-end)*sizeof(char *));
    SC->count -= end - start;
    return 1;
}

static int EraseInternal(ElementType *SC,const CHAR_TYPE *str,int all)
{
    size_t i;
    int result = CONTAINER_ERROR_NOTFOUND;
    if (SC == NULL) {
        return NullPtrError("Erase");
    }
    if (str == NULL) {
        return BadArgError(SC,"Erase");
    }
    if (SC->Flags & CONTAINER_READONLY)
        return ReadOnlyError(SC,"Erase");
    for (i=0; i<SC->count;i++) {
        if (!SC->strcompare((const void **)&SC->contents[i],
                            (const void **)&str,SC->StringCompareContext)) {
            if (SC->DestructorFn) SC->DestructorFn(SC->contents[i]);
            SC->Allocator->free(SC->contents[i]);
            if (i < (SC->count-1))
                memmove(SC->contents+i,SC->contents+i+1,(SC->count-i)*sizeof(char *));
            --SC->count;
            SC->contents[SC->count]=NULL;
            SC->timestamp++;
            result = 1;
            if (all == 0) break;
            i--;
        }
    }
    return result;
}

static int Erase(ElementType *SC,const CHAR_TYPE *str)
{
    return EraseInternal(SC,str,0);
}

static int EraseAll(ElementType *SC,const CHAR_TYPE *str)
{
    return EraseInternal(SC,str,1);
}

static int PushBack(ElementType *SC,const CHAR_TYPE *str)
{
    CHAR_TYPE *r;

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

static int PushFront(ElementType *SC,CHAR_TYPE *str)
{
    CHAR_TYPE *r;

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

static size_t PopBack(ElementType *SC,CHAR_TYPE *buffer,size_t buflen)
{
    CHAR_TYPE *result;
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

static size_t PopFront(ElementType *SC,CHAR_TYPE *buffer,size_t buflen)
{
    CHAR_TYPE *result;
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
        memmove(SC->contents,SC->contents+1,SC->count*sizeof(void *));
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


static size_t GetCapacity(const ElementType *SC)
{
    if (SC == NULL) {
        NullPtrError("SetCapacity");
        return 0;
    }
    return SC->capacity;
}

static int SetCapacity(ElementType *SC,size_t newCapacity)
{
    CHAR_TYPE **newContents;
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

static int Apply(ElementType *SC,int (*Applyfn)(CHAR_TYPE *,void *),void *arg)
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

/* gedd123@free.fr (gerome) proposed calling DuplicateString. Good
suggestion */
static int ReplaceAt(ElementType *SC,size_t idx,CHAR_TYPE *newval)
{
    CHAR_TYPE *r;

    if (SC == NULL) {
        return NullPtrError("ReplaceAt");
    }
    if (SC->Flags & CONTAINER_READONLY) {
        return ReadOnlyError(SC,"ReplaceAt");
    }
    if (idx >= SC->count) {
        return IndexError(SC,idx,"ReplaceAt");
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

static int Equal(const ElementType *SC1,const ElementType *SC2)
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
        if (SC1->strcompare((const void **)&SC1->contents[i],(const void **)&SC2->contents[i],ci))
            return 0;
    }
    return 1;
}

static ElementType *Copy(const ElementType *SC)
{
    size_t i;
    ElementType *result;

    if (SC == NULL) {
        NullPtrError("Copy");
        return NULL;
    }
    result = iElementType.Create(SC->count);
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


static ErrorFunction SetErrorFunction(ElementType *SC,ErrorFunction fn)
{
    ErrorFunction old;
    if (SC == NULL) {
        return iError.RaiseError;
    }
    old = SC->RaiseError;
    if (fn)
        SC->RaiseError = (fn);
    return old;
}

static int Sort(ElementType *SC)
{
    CompareInfo ci;

    ci.ExtraArgs = NULL;
    ci.ContainerLeft = SC;
    ci.ContainerRight = NULL;
    if (SC == NULL) {
        return NullPtrError("Sort");
    }
    if (SC->Flags & CONTAINER_READONLY) {
        return ReadOnlyError(SC,"Sort");
    }
    qsortEx(SC->contents,SC->count,sizeof(char *),(CompareFunction)SC->strcompare,&ci);
    SC->timestamp++;
    return 1;
}

static size_t Sizeof(const ElementType *SC)
{
    size_t result= sizeof(ElementType);
    size_t i;

    if (SC == NULL) {
        return sizeof(ElementType);
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

struct strCollectionIterator {
    Iterator it;
    ElementType *SC;
    size_t index;
    unsigned timestamp;
    unsigned long Flags;
    CHAR_TYPE *current;
};

static size_t GetPosition(Iterator *it)
{
    struct VectorIterator *ali = (struct VectorIterator *)it;
    return ali->index;
}

static void *GetNext(Iterator *it)
{
    struct strCollectionIterator *sci = (struct strCollectionIterator *)it;
    ElementType *SC;

    if (sci == NULL) {
        NullPtrError("GetNext");
        return NULL;
    }
    SC = sci->SC;
    if (SC->count == 0)
    return NULL;
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
    struct strCollectionIterator *ali = (struct strCollectionIterator *)it;
    ElementType *SC;

    if (ali == NULL) {
        NullPtrError("GetPrevious");
        return NULL;
    }
    SC = ali->SC;
    if (SC->count == 0 || ali->index >= SC->count || ali->index == 0)
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
    struct strCollectionIterator *ali = (struct strCollectionIterator *)it;

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

static void *Seek(Iterator *it,size_t idx)
{
        struct strCollectionIterator *ali = (struct strCollectionIterator *)it;
        ElementType *AL;
        CHAR_TYPE *p;

        if (ali == NULL) {
                NullPtrError("Seek");
                return NULL;
        }
        AL = ali->SC;
        if (idx >= AL->count)
                return NULL;
        if (ali->timestamp != AL->timestamp) {
                AL->RaiseError("Seek",CONTAINER_ERROR_OBJECT_CHANGED);
                return NULL;
        }
        p = AL->contents[idx];
        ali->index = idx;
        ali->current = p;
        return p;
}


static void *GetCurrent(Iterator *it)
{
    struct strCollectionIterator *ali = (struct strCollectionIterator *)it;
    
    if (ali == NULL) {
        NullPtrError("GetCurrent");
        return NULL;
    }
    return ali->current;
}

static int ReplaceWithIterator(Iterator *it, void *data,int direction) 
{
    struct strCollectionIterator *ali = (struct strCollectionIterator *)it;
    int result;
    size_t pos;
    
    if (it == NULL) {
        return NullPtrError("Replace");
    }
    if (ali->SC->count == 0)
        return 0;
    if (ali->timestamp != ali->SC->timestamp) {
        ali->SC->RaiseError("Replace",CONTAINER_ERROR_OBJECT_CHANGED);
        return CONTAINER_ERROR_OBJECT_CHANGED;
    }
    if (ali->SC->Flags & CONTAINER_READONLY) {
        ali->SC->RaiseError("Replace",CONTAINER_ERROR_READONLY);
        return CONTAINER_ERROR_READONLY;
    }    
    pos = ali->index;
    if (direction)
        GetNext(it);
    else
        GetPrevious(it);
    if (data == NULL)
        result = RemoveAt(ali->SC,pos);
    else {
        result = ReplaceAt(ali->SC,pos,data);
    }
    if (result >= 0) {
        ali->timestamp = ali->SC->timestamp;
    }
    return result;
}


static Iterator *NewIterator(ElementType *SC)
{
    struct strCollectionIterator *result;

    result  = SC->Allocator->malloc(sizeof(struct strCollectionIterator));
    if (result == NULL) {
        NoMemoryError(SC,"NewIterator");
        return NULL;
    }
    result->it.GetNext = GetNext;
    result->it.GetPrevious = GetPrevious;
    result->it.GetFirst = GetFirst;
    result->it.GetCurrent = GetCurrent;
    result->it.Seek = Seek;
    result->it.Replace = ReplaceWithIterator;
    result->it.GetPosition = GetPosition;
    result->SC = SC;
    result->timestamp = SC->timestamp;
    result->current = NULL;
    return &result->it;
}
static int InitIterator(ElementType *SC,void *buf)
{
    struct strCollectionIterator *result=buf;

    result->it.GetNext = GetNext;
    result->it.GetPrevious = GetPrevious;
    result->it.GetFirst = GetFirst;
    result->it.GetCurrent = GetCurrent;
    result->it.Seek = Seek;
    result->it.Replace = ReplaceWithIterator;
    result->SC = SC;
    result->timestamp = SC->timestamp;
    result->current = NULL;
    return 1;
}


static int deleteIterator(Iterator *it)
{
    struct strCollectionIterator *sci = (struct strCollectionIterator *)it;
    ElementType *SC;

    if (sci == NULL) {
        return NullPtrError("deleteIterator");
    }
    SC = sci->SC;
    SC->Allocator->free(it);
    return 1;
}

static int SaveHeader(const ElementType *SC,FILE *stream)
{
    return (int)fwrite(SC,1,sizeof(ElementType),stream);
}

static int DefaultSaveFunction(const void *element,void *arg, FILE *Outfile)
{
    const CHAR_TYPE *str = (const CHAR_TYPE *)element;
    size_t len = STRLEN(str);

    if (encode_ule128(Outfile, len) <= 0)
        return EOF;
    len++;
    return len == fwrite(str,1,len,Outfile);
}

static int DefaultLoadFunction(void *element,void *arg, FILE *Infile)
{
    size_t len = *(size_t *)arg;

    return len == fread(element,1,len,Infile);
}

static int Save(const ElementType *SC,FILE *stream, SaveFunction saveFn,void *arg)
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
    if (fwrite(&strCollectionGuid,sizeof(guid),1,stream) == 0)
        return EOF;
    if (SaveHeader(SC,stream) <= 0)
        return EOF;
    for (i=0; i< SC->count; i++) {
        if (saveFn(SC->contents[i],arg,stream) <= 0)
            return EOF;
    }
    return 1;
}

static ElementType *Load(FILE *stream, ReadFunction readFn,void *arg)
{
    size_t i,len=0;
    ElementType *result,SC;
    guid Guid;

    if (stream == NULL) {
        NullPtrError("Load");
        return NULL;
    }
    if (readFn == NULL) {
        readFn = DefaultLoadFunction;
        arg = &len;
    }
    if (fread(&Guid,sizeof(guid),1,stream) == 0) {
        iError.RaiseError("istrCollection.Load",CONTAINER_ERROR_FILE_READ);
        return NULL;
    }
    if (memcmp(&Guid,&strCollectionGuid,sizeof(guid))) {
        iError.RaiseError("istrCollection.Load",CONTAINER_ERROR_WRONGFILE);
        return NULL;
    }
    if (fread(&SC,1,sizeof(ElementType),stream) == 0) {
        iError.RaiseError("istrCollection.Load",CONTAINER_ERROR_FILE_READ);
        return NULL;
    }
    result = iElementType.Create(SC.count);
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
            iError.RaiseError("ElementType.Load",CONTAINER_ERROR_FILE_READ);
            break;
        }
        result->count++;
    }
    return result;
}
static Vector *CastToArray(const ElementType *SC)
{
    Vector *AL = iVector.Create(sizeof(void *),SC->count);
    size_t i;

    for (i=0; i<SC->count;i++) {
        iVector.Add(AL,SC->contents[i]);
    }
    return AL;
}

/* Bug fixes proposed by oetelaar.automatisering */
static ElementType *CreateFromFile(const char *fileName)
{
    ElementType *result;
    CHAR_TYPE *line=NULL;
    int llen=0,r;
    ContainerAllocator *mm = CurrentAllocator;
    FILE *f;

    if (fileName == NULL) {
        NullPtrError("CreateFromFile");
        return NULL;
    }
    f = fopen((char *)fileName,"r");
    if (f == NULL) {
        iError.RaiseError("istrCollection.CreateFromFile",CONTAINER_ERROR_NOENT);
        return NULL;
    }
    result = iElementType.Create(10);
    if (result == NULL) {
        fclose(f); /* Was missing! */
        return NULL;
    }
    r = GETLINE(&line,&llen,f,mm);
    while (r >= 0) {
        if (iElementType.Add(result,line) <= 0) {
            Finalize(result);
            free(line); /* was missing! */
            fclose(f);
            return NULL;
        }
        r = GETLINE(&line,&llen,f,mm);
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

static ElementType *GetRange(ElementType *SC, size_t start,size_t end)
{
    ElementType *result;
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


static int WriteToFile(const ElementType *SC,const  char *fileName)
{
    FILE *f;
    size_t i;
    int result = 0;
    CHAR_TYPE nl[2];

    if (SC == NULL || fileName == NULL) {
        return NullPtrError("WriteToFile");
    }
    f = fopen(fileName,"w");
    if (f == NULL) {
        SC->RaiseError("istrCollection.WriteToFile",CONTAINER_ERROR_FILEOPEN);
        return CONTAINER_ERROR_FILEOPEN;
    }
    nl[0] = NEWLINE;
    for (i=0; i<SC->count;i++) {
        if (SC->contents[i][0] && fwrite(SC->contents[i],1,strlen((char *)SC->contents[i]),f) == 0) {
writeerror:
            iError.RaiseError("istrCollection.WriteToFile",CONTAINER_ERROR_FILE_WRITE);
            result = CONTAINER_ERROR_FILE_WRITE;
            break;
        }
        if (fwrite(nl,1,sizeof(CHAR_TYPE),f) == 0)
            goto writeerror;
    }
    if (i == SC->count && i > 0)
        result = 1;
    fclose(f);
    return result;
}

static size_t FindFirst(const ElementType *SC,const CHAR_TYPE *text)
{
    size_t i;

    if (SC == NULL || text == NULL)
        return 0;
    for (i=0; i<SC->count;i++) {
        if (STRSTR(SC->contents[i],text)) {
            return i+1;
        }
    }
    return 0;
}

static StringCompareFn SetCompareFunction(ElementType *SC,StringCompareFn fn)
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

static size_t FindNext(const ElementType *SC,const CHAR_TYPE *text, size_t start)
{
    size_t i;

    if (SC == NULL || text == NULL)
        return 0;
    for (i=start; i<SC->count;i++) {
        if (STRSTR(SC->contents[i],text)) {
            return i+1;
        }
    }
    return 0;
}

static ElementType *FindText(const ElementType *SC,const CHAR_TYPE *text)
{
    ElementType *result = NULL;
    size_t i;

    for (i=0; i<SC->count;i++) {
        if (STRSTR(SC->contents[i],text)) {
            if (result == NULL) {
                result = iElementType.Create(sizeof(size_t));
                if (result == NULL)
                    return NULL;
            }
            if (iElementType.Add(result,SC->contents[i]) <= 0)
                break;
        }
    }
    return result;
}

static Vector *FindTextIndex(const ElementType *SC,const CHAR_TYPE *text)
{
    Vector *result = NULL;
    size_t i;

    for (i=0; i<SC->count;i++) {
        if (STRSTR(SC->contents[i],text)) {
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

static Vector *FindTextPositions(const ElementType *SC,const CHAR_TYPE *text)
{
    Vector *result = NULL;
    CHAR_TYPE *p;
    size_t i,idx;

    for (i=0; i<SC->count;i++) {
        if (NULL != (p=STRSTR(SC->contents[i],text))) {
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
static int Strcmp(const void **s1,const void **s2, CompareInfo *info)
{
    return STRCMP(*s1,*s2);
}
/*------------------------------------------------------------------------
 Procedure:     Create ID:1
 Purpose:       Creates a new string collection
 Input:         The initial start size of the collection
 Output:        A pointer to the new collection or NULL
 Errors:        If no more memory is available it returns NULL
 after calling the error function.
 ------------------------------------------------------------------------*/
static ElementType *InitWithAllocator(ElementType *result,size_t startsize,const ContainerAllocator *allocator)
{
    memset(result,0,sizeof(ElementType));
    result->VTable = &iElementType;
    if (startsize > 0) {
        result->contents = allocator->malloc(startsize*sizeof(char *));
        if (result->contents == NULL) {
            iError.RaiseError("istrCollection.Create",CONTAINER_ERROR_NOMEMORY);
            return NULL;
        }
        else {
            memset(result->contents,0,sizeof(char *)*startsize);
            result->capacity = startsize;
        }
    }
    result->RaiseError = iError.RaiseError;
    result->strcompare = Strcmp;
    result->Allocator = (ContainerAllocator *)allocator;
    return result;

}
static ElementType  *CreateWithAllocator(size_t startsize,const ContainerAllocator *allocator)
{
    ElementType *result,*r1;

    r1 = allocator->malloc(sizeof(*result));
    if (r1 == NULL) {
        iError.RaiseError("istrCollection.Create",CONTAINER_ERROR_NOMEMORY);
        return NULL;
    }
    result = InitWithAllocator(r1,startsize,allocator);
    if (r1 == NULL) {
        allocator->free(result);
    }
    return result;
}

static ElementType *Init(ElementType *result,size_t startsize)
{
    return InitWithAllocator(result,startsize,CurrentAllocator);
}

static ElementType *Create(size_t startsize)
{
    return CreateWithAllocator(startsize,CurrentAllocator);
}

static ElementType *InitializeWith(size_t n, CHAR_TYPE **data)
{
    size_t i;
    ElementType *result = Create(n);
    if (result == NULL) return result;
    for (i=0; i<n; i++) {
        result->contents[i] = DuplicateString(result,data[i],"InitializeWith");
        if (result->contents[i] == NULL) {
            Finalize(result);
            return NULL;
        }
    }
    result->count = n;
    return result;
}

static size_t GetElementSize(const ElementType *sc)
{
    return sizeof(void *);
}

/* Proposed by PWO 
*/

static int Reverse(ElementType *SC)
{
    CHAR_TYPE **p, **q, *t;

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

static DestructorFunction SetDestructor(ElementType *cb,DestructorFunction fn)
{
    DestructorFunction oldfn;
    if (cb == NULL)
        return NULL;
    oldfn = cb->DestructorFn;
    if (fn)
        cb->DestructorFn = fn;
    return oldfn;
}

static CHAR_TYPE **GetData(const ElementType *cb)
{
    if (cb == NULL) {
        NullPtrError("GetData");
        return NULL;
    }
    if (cb->Flags&CONTAINER_READONLY) {
        cb->RaiseError("GetData",CONTAINER_ERROR_READONLY);
        return NULL;
    }
    return cb->contents;
}

static CHAR_TYPE *Back(const ElementType *cb)
{
    if (cb == NULL) {
        NullPtrError("Back");
        return NULL;
    }
    if (cb->Flags&CONTAINER_READONLY) {
        cb->RaiseError("Back",CONTAINER_ERROR_READONLY);
        return NULL;
    }
    if (cb->count == 0)
        return NULL;
    return cb->contents[cb->count-1];
}    
static CHAR_TYPE *Front(const ElementType *cb)
{
    if (cb == NULL) {
        NullPtrError("Front");
        return NULL;
    }
    if (cb->Flags&CONTAINER_READONLY) {
        cb->RaiseError("Front",CONTAINER_ERROR_READONLY);
        return NULL;
    }
    if (cb->count == 0)
        return NULL;
    return cb->contents[0];
}
static size_t SizeofIterator(const ElementType *ignored)
{
    return sizeof(struct strCollectionIterator);
}

static Mask *CompareEqual(const ElementType *left,const ElementType *right,Mask *bytearray)
{
    
    size_t left_len;
    size_t right_len;
    size_t i;
    
    if (left == NULL || right == NULL) {
        NullPtrError("CompareEqual");
        return NULL;
    }
    left_len = left->count; right_len = right->count;
    if (left_len != right_len) {
        iError.RaiseError("iVector.CompareEqual",CONTAINER_ERROR_INCOMPATIBLE);
        return NULL;
    }
    if (bytearray == NULL || iMask.Size(bytearray) < left_len) {
        if (bytearray) iMask.Finalize(bytearray);
        bytearray = iMask.Create(left_len);
    }
    else iMask.Clear(bytearray);
    if (bytearray == NULL) {
        iError.RaiseError("CompareEqual",CONTAINER_ERROR_NOMEMORY);
        return NULL;
    }
    for (i=0; i<left_len;i++) {
        bytearray->data[i] = !STRCMP(left->contents[i],right->contents[i]);
    }
    return bytearray;
}

static Mask *CompareEqualScalar(const ElementType *left,const CHAR_TYPE *right,Mask *bytearray)
{
    
    size_t left_len;
    size_t i;
    
    if (left == NULL || right == NULL) {
        NullPtrError("CompareEqual");
        return NULL;
    }
    left_len = left->count;
    if (bytearray == NULL || iMask.Size(bytearray) < left_len) {
        if (bytearray) iMask.Finalize(bytearray);
        bytearray = iMask.Create(left_len);
        if (bytearray == NULL) {
       	    iError.RaiseError("CompareEqual",CONTAINER_ERROR_NOMEMORY);
            return NULL;
        }
    }
    else iMask.Clear(bytearray);
    if (bytearray == NULL) {
        iError.RaiseError("CompareEqual",CONTAINER_ERROR_NOMEMORY);
        return NULL;
    }
    for (i=0; i<left_len;i++) {
        bytearray->data[i] = !STRCMP(left->contents[i],right);
    }
    return bytearray;
}

static int Select(ElementType *src,const Mask *m)
{
    size_t i,offset=0;

    if (src == NULL || m == NULL)
        return NullPtrError("Select");
    if (m->length != src->count) {
        iError.RaiseError("iVector.Select",CONTAINER_ERROR_BADMASK,src,m);
        return CONTAINER_ERROR_BADMASK;
    }
    for (i=0; i<m->length;i++) {
        if (m->data[i]) {
            if (i != offset) {
                if (src->DestructorFn) src->DestructorFn(src->contents[offset]);
                src->Allocator->free(src->contents[offset]);
                src->contents[offset] = src->contents[i];
            }
            offset++;
        }
    }
    src->count = offset;
    if (offset < i) {
        while (offset < i) {
            if (src->DestructorFn) src->DestructorFn(src->contents[offset]);
            src->Allocator->free(src->contents[offset]);
            src->contents[offset] = NULL;
            offset++;
        }
    }
    return 1;
}

static ElementType  *SelectCopy(const ElementType *src,const Mask *m)
{
    size_t i,offset=0;
    ElementType *result;
    
    if (m->length != src->count) {
        iError.RaiseError("iVector.SelectCopy",CONTAINER_ERROR_BADMASK,src,m);
        return NULL;
    }
    result = Create(src->count);
    if (result == NULL) {
        NoMemoryError(src,"SelectCopy");
        return NULL;
    }
    for (i=0; i<m->length;i++) {
        if (m->data[i]) {
            result->contents[offset] = DuplicateString(src,src->contents[i],"SelectCopy");
            if (result->contents[offset] == NULL) {
                Finalize(result);
                return NULL;
            }
            offset++;
            result->count++;
        }
    }
    return result;
}

INTERFACE_TYP INTERFACE_OBJECT = {
    GetCount, 
    GetFlags, 
    SetFlags,  
    Clear,
    Contains,
    Erase,
    EraseAll,
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
    PushFront,
    PopFront,
    InsertAt,
    RemoveAt,
    ReplaceAt,
    IndexOf,
    Sort,
    CastToArray,
    FindFirst,
    FindNext,
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
    SetDestructor,
    InitializeWith,
    GetData,
    Back,
    Front,
    RemoveRange,
    CompareEqual,
    CompareEqualScalar,
    Select,
    SelectCopy,
};
