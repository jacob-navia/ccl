#include "containers.h"
#include "ccl_internal.h"

static size_t Size(const GenericContainer *gen)
{
	if (gen == NULL) {
		iError.RaiseError("iGeneric.Size",CONTAINER_ERROR_BADARG);
		return 0;
	}
	return gen->vTable->Size(gen);
}

static unsigned GetFlags(const GenericContainer  *gen)
{
	if (gen == NULL) {
		iError.RaiseError("iGneric.GetFlags",CONTAINER_ERROR_BADARG);
		return 0;
	}
	return gen->vTable->GetFlags(gen);
}

static unsigned SetFlags(GenericContainer *gen,unsigned newFlags)
{
	unsigned result;
	if (gen == NULL) {
		iError.RaiseError("iGeneric.SetFlags",CONTAINER_ERROR_BADARG);
		return 0;
	}
	result = gen->vTable->GetFlags(gen);
	gen->vTable->SetFlags(gen,newFlags);
	return result;
}

static int Clear(GenericContainer *gen)
{
	return gen->vTable->Clear(gen);
}

static int Contains(const GenericContainer *gen,const void *value)
{
	return gen->vTable->Contains(gen,value);
}

static int Erase(GenericContainer *gen,const void *elem)
{
	return gen->vTable->Erase(gen,elem);
}

static int EraseAll(GenericContainer *gen,const void *elem)
{
        return gen->vTable->EraseAll(gen,elem);
}

static int Finalize(GenericContainer *gen)
{
	return gen->vTable->Finalize(gen);
}

static void Apply(GenericContainer *Gen,int (*Applyfn)(void *,void * arg),void *arg)
{
	Gen->vTable->Apply(Gen,Applyfn,arg);
}

static int Equal(const GenericContainer *Gen1,const GenericContainer *Gen2)
{
	return Gen1->vTable->Equal(Gen1,Gen2);
}

static GenericContainer *Copy(const GenericContainer *src)
{
	return src->vTable->Copy(src);
}

static ErrorFunction SetErrorFunction(GenericContainer *Gen,ErrorFunction fn)
{
	return Gen->vTable->SetErrorFunction(Gen,fn);
}

static size_t Sizeof(const GenericContainer *gen)
{
	return gen->vTable->Sizeof(gen);
}


static Iterator *NewIterator(GenericContainer *gen)
{
	return gen->vTable->NewIterator(gen);
}


static size_t SizeofIterator(const GenericContainer *gen)
{
	return gen->vTable->SizeofIterator(gen);
}
static int InitIterator(GenericContainer *gen,void *buf)
{
        return gen->vTable->InitIterator(gen,buf);
}



static int deleteIterator(Iterator *git)
{
	GenericIterator *GenIt = (GenericIterator *)git;
	GenericContainer *gen = GenIt->Gen;
	return gen->vTable->deleteIterator(git);
}

static int Save(const GenericContainer *gen, FILE *stream,SaveFunction saveFn,void *arg)
{
	return gen->vTable->Save(gen,stream,saveFn,arg);
}

GenericContainerInterface iGeneric = {
Size,
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
};

