#include "containers.h"
#include "ccl_internal.h"
#include "vectorgen.h"
#ifndef DEFAULT_START_SIZE
#define DEFAULT_START_SIZE 20
#endif

static VECTOR_TYPE *SetVTable(VECTOR_TYPE *result);

static int NullPtrError(const char *fnName)
{
	char buf[512];

	snprintf(buf,sizeof(buf),"iVector.%s",fnName);
	return iError.NullPtrError(buf);
}

static int Add(VECTOR_TYPE * l, const DATA_TYPE elem)
{
    return iVector.Add((Vector *)l,&elem);
}

static VECTOR_TYPE *GetRange(const VECTOR_TYPE *AL, size_t start,size_t end)
{
	VECTOR_TYPE *result=(VECTOR_TYPE *)iVector.GetRange((Vector *)AL,start,end);
	return SetVTable(result);
}

static int Contains(const VECTOR_TYPE * l, const DATA_TYPE data,void *ExtraArgs)
{
    size_t idx;
    return (iVector.IndexOf((Vector *)l, &data, ExtraArgs, &idx) < 0) ? 0 : 1;
}

static VECTOR_TYPE  *Copy(const VECTOR_TYPE * l)
{
    VECTOR_TYPE *result = (VECTOR_TYPE *)iVector.Copy((Vector *)l);
    return SetVTable(result);
}


static void **CopyTo(const VECTOR_TYPE *AL)
{
	return iVector.CopyTo((Vector *)AL);
}

static int IndexOf(const VECTOR_TYPE *AL,const DATA_TYPE data,void *ExtraArgs,size_t *result)
{
	return iVector.IndexOf((Vector *)AL,&data,ExtraArgs,result);
}

static int InsertAt(VECTOR_TYPE *AL,size_t idx,DATA_TYPE newval)
{
	return iVector.InsertAt((Vector *)AL, idx,&newval);;
}

static int Insert(VECTOR_TYPE *AL,DATA_TYPE newval)
{
	return iVector.InsertAt((Vector *)AL,0,&newval);
}

static int Erase(VECTOR_TYPE *AL, DATA_TYPE elem)
{
	return iVector.Erase((Vector *)AL, &elem);
}
static int EraseAll(VECTOR_TYPE *AL, DATA_TYPE elem)
{
    return iVector.EraseAll((Vector *)AL, &elem);;
}

static int PushBack(VECTOR_TYPE *AL,const DATA_TYPE data)
{
	return iVector.PushBack((Vector *)AL,&data);
}

static int PopBack(VECTOR_TYPE *AL,DATA_TYPE result)
{
    return iVector.PushBack((Vector *)AL,&result);;
}

/*------------------------------------------------------------------------
 Procedure:     ReplaceAt ID:1
 Purpose:       Replace the data at the given position with new
                data.
 Input:         The list and the new data
 Output:        Returns the new data if OK, NULL if error.
 Errors:        Index error or read only errors are caught
------------------------------------------------------------------------*/
static int ReplaceAt(VECTOR_TYPE *AL,size_t idx,DATA_TYPE newval)
{
	return iVector.ReplaceAt((Vector *)AL,idx,&newval);
}

static size_t Sizeof(const VECTOR_TYPE *AL)
{
	if (AL == NULL)
		return sizeof(VECTOR_TYPE);
	return sizeof(VECTOR_TYPE) + (AL->count * sizeof(DATA_TYPE));
}

static int Sort(VECTOR_TYPE *AL)
{
	CompareInfo ci;

	if (AL == NULL) {
		return NullPtrError("Sort");
	}
	ci.ContainerLeft = AL;
	ci.ExtraArgs = NULL;
	qsortEx(AL->contents,AL->count,AL->ElementSize,AL->CompareFn,&ci);
	return 1;
}


/* ------------------------------------------------------------------------------ */
/*                                Iterators                                       */
/* ------------------------------------------------------------------------------ */


static int ReplaceWithIterator(Iterator * it, DATA_TYPE data, int direction)
{
    struct ITERATOR(DATA_TYPE) *iter = (struct ITERATOR(DATA_TYPE) *)it;
    return iter->VectorReplace(it,&data,direction);
}

static Iterator *SetupIteratorVTable(struct ITERATOR(DATA_TYPE) *result)
{
    if (result == NULL) return NULL;
    result->VectorReplace = result->it.Replace;
    result->it.Replace = (int (*)(Iterator * , void * , int ))ReplaceWithIterator;
    return &result->it;
}


static Iterator *NewIterator(VECTOR_TYPE * L)
{
    return SetupIteratorVTable((struct ITERATOR(DATA_TYPE) *)iVector.NewIterator((Vector *)L));
}


static VECTOR_TYPE *Load(FILE * stream, ReadFunction loadFn, void *arg)
{
    VECTOR_TYPE *result = (VECTOR_TYPE *)iVector.Load(stream,loadFn,arg);
    return SetVTable(result);
}

static int InitIterator(VECTOR_TYPE * L, void *r)
{
    iVector.InitIterator((Vector *)L,r);
    SetupIteratorVTable(r);
    return 1;
}


static VECTOR_TYPE *SetVTable(VECTOR_TYPE *result)
{
	static int Initialized;
	INTERFACE(DATA_TYPE) *intface = &INTERFACE_NAME(DATA_TYPE);

	result->VTable = intface;
	if (Initialized) return result;
	Initialized = 1;
	intface->Size = (size_t (*)(const VECTOR_TYPE *))iVector.Size;
	intface->SetFlags = (unsigned (*)(VECTOR_TYPE * l, unsigned newval))iVector.SetFlags;
	intface->GetFlags = (unsigned (*)(const VECTOR_TYPE *))iVector.GetFlags;
	intface->GetCapacity = (size_t (*)(const VECTOR_TYPE *))iVector.GetCapacity;
	intface->SetCapacity = (int (*)(VECTOR_TYPE *, size_t))iVector.SetCapacity;
	intface->Clear = (int (*)(VECTOR_TYPE *))iVector.Clear;
	intface->Finalize = (int (*)(VECTOR_TYPE *))iVector.Finalize;
	intface->Apply = (int (*)(VECTOR_TYPE *, int (Applyfn) (DATA_TYPE *, void * ), void *))iVector.Apply;
	intface->SetErrorFunction = (ErrorFunction (*)(VECTOR_TYPE *, ErrorFunction))iVector.SetErrorFunction;
	intface->Resize = (int (*)(VECTOR_TYPE *,size_t))iVector.Resize;
	intface->Save = (int (*)(const VECTOR_TYPE *, FILE *, SaveFunction, void *))iVector.Save;
	intface->IndexIn = (VECTOR_TYPE *(*)(VECTOR_TYPE *,const Vector *))iVector.IndexIn;
	intface->EraseAt = (int (*)(VECTOR_TYPE *AL,size_t idx))iVector.EraseAt;
	intface->InsertIn = (int (*)(VECTOR_TYPE *, size_t,VECTOR_TYPE *))iVector.InsertIn;
	intface->Append = (int (*)(VECTOR_TYPE *, VECTOR_TYPE *))iVector.Append;
	intface->RemoveRange = (int (*)(VECTOR_TYPE *,size_t,size_t))iVector.RemoveRange;
	intface->AddRange = (int (*)(VECTOR_TYPE *,size_t,const DATA_TYPE *))iVector.AddRange;
	intface->GetAllocator = (const ContainerAllocator * (*)(const VECTOR_TYPE *))iVector.GetAllocator;
	intface->Mismatch = (int (*)(VECTOR_TYPE *a1, VECTOR_TYPE *a2,size_t *mismatch))iVector.Mismatch;
	intface->SetCompareFunction = (CompareFunction (*)(VECTOR_TYPE *,CompareFunction))iVector.SetCompareFunction;
	intface->SelectCopy = (VECTOR_TYPE *(*)(VECTOR_TYPE *,Mask *))iVector.SelectCopy;
	intface->CopyElement = (int (*)(const VECTOR_TYPE *,size_t,DATA_TYPE *))iVector.CopyElement;
	intface->Reverse = (int (*)(VECTOR_TYPE *))iVector.Reverse;
	intface->SetDestructor = (DestructorFunction (*)(VECTOR_TYPE *,DestructorFunction))iVector.SetDestructor;
	intface->SearchWithKey = (int (*)(VECTOR_TYPE *,size_t,size_t ,size_t,DATA_TYPE,size_t *))iVector.SearchWithKey;
	intface->Select = (int (*)(VECTOR_TYPE *src,const Mask *m))iVector.Select;
	intface->GetData = (DATA_TYPE *(*)(const VECTOR_TYPE *))iVector.GetData;
	intface->RotateLeft = (int (*)(VECTOR_TYPE *V,size_t n))iVector.RotateLeft;
	intface->RotateRight = (int (*)(VECTOR_TYPE *V,size_t n))iVector.RotateRight;
	intface->CompareEqual = (Mask *(*)(const VECTOR_TYPE *,const VECTOR_TYPE *,Mask *))iVector.CompareEqual;
	intface->GetElement = (DATA_TYPE *(*)(const VECTOR_TYPE *,size_t))iVector.GetElement;
	intface->Back = (DATA_TYPE *(*)(const VECTOR_TYPE *))iVector.Back;
	intface->Front = (DATA_TYPE *(*)(const VECTOR_TYPE *))iVector.Front;

	return result;
}

static VECTOR_TYPE *CreateWithAllocator(size_t siz,const ContainerAllocator *a)
{
	VECTOR_TYPE *r = (VECTOR_TYPE *)iVector.CreateWithAllocator(sizeof(DATA_TYPE),siz,a);
	if (r == NULL) return NULL;
	return SetVTable(r);
}

static VECTOR_TYPE *Create(size_t startsize)
{
	return CreateWithAllocator(startsize,CurrentAllocator);
}

static VECTOR_TYPE *InitializeWith(size_t n,const DATA_TYPE *data)
{
        VECTOR_TYPE *result = Create(n);
        if (result) {
        	memcpy(result->contents,data,n*sizeof(DATA_TYPE));
        	result->count = n;
	}
        return result;
}


static VECTOR_TYPE *Init(VECTOR_TYPE *result,size_t startsize)
{
	VECTOR_TYPE *r =  (VECTOR_TYPE *)iVector.Init((Vector *)result,sizeof(DATA_TYPE),startsize);
	return SetVTable(r);
}

static size_t GetElementSize(const VECTOR_TYPE *AL)
{
	return sizeof(DATA_TYPE);
}

static size_t SizeofIterator(const VECTOR_TYPE * l)
{
    return sizeof(struct ITERATOR(DATA_TYPE));
}

static Mask *CompareEqualScalar(const VECTOR_TYPE *left,const DATA_TYPE right,Mask *bytearray)
{
    return iVector.CompareEqualScalar((Vector *)left,&right,bytearray);
}

INTERFACE(DATA_TYPE)   INTERFACE_NAME(DATA_TYPE) = {
	NULL,        // Size,
	NULL,        // GetFlags, 
	NULL,        // SetFlags, 
	NULL,        // Clear,
	Contains,
	Erase, 
        EraseAll,
	NULL,        // Finalize,
	NULL,        // Apply,
	NULL,        // Equal,
	Copy,
	NULL,        // SetErrorFunction,
	Sizeof,
	NewIterator,
	InitIterator,
	NULL,        // deleteIterator,
	SizeofIterator,
	NULL,       // Save,
	Load,
	GetElementSize,
/* Sequential container fields */
	Add, 
	NULL,       // GetElement,
	PushBack,
	PopBack,
	InsertAt,
	NULL,       // EraseAt, 
	ReplaceAt,
	IndexOf, 
/* Vector specific fields */
	Insert,
	NULL,       // InsertIn,
	NULL,       // IndexIn,
	NULL,       // GetCapacity, 
	NULL,       // SetCapacity,
	NULL,       // SetCompareFunction,
	Sort,
	Create,
	CreateWithAllocator,
	Init,
	NULL,       // AddRange, 
	GetRange,
	NULL,       // CopyElement,
	CopyTo, 
	NULL,        // Reverse,
	NULL,        // Append,
	NULL,        // Mismatch,
	NULL,        // GetAllocator,
	NULL,        // SetDestructor,
	NULL,        // SearchWithKey,
	NULL,        // Select,
	NULL,        //SelectCopy,
	NULL,        // Resize,
	InitializeWith,
	NULL,        // GetData,
	NULL,        // Back,
	NULL,        // Front,
	NULL,       // RemoveRange,
	NULL,       // RotateLeft,
	NULL,       // RotateRight,
	NULL,       // CompareEqual,
	CompareEqualScalar,
	NULL,       // Reserve
};
