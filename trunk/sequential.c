#include "containers.h"
#include "ccl_internal.h"

struct SequentialContainer {
	SequentialContainerInterface *vTable;
	size_t Size;
	unsigned Flags;
	size_t ElementSize;
};

static size_t Size(const SequentialContainer *gen)
{
	if (gen == NULL) {
		iError.RaiseError("iGeneric.Size",CONTAINER_ERROR_BADARG);
		return 0;
	}
	return gen->Size;
}

static unsigned GetFlags(const SequentialContainer  *gen)
{
	if (gen == NULL) {
		iError.RaiseError("iGneric.GetFlags",CONTAINER_ERROR_BADARG);
		return 0;
	}
	return gen->Flags;
}

static unsigned SetFlags(SequentialContainer *gen,unsigned newFlags)
{
	unsigned result;
	if (gen == NULL) {
		iError.RaiseError("iGeneric.SetFlags",CONTAINER_ERROR_BADARG);
		return 0;
	}
	result = gen->Flags;
	gen->Flags = newFlags;
	return result;
}

static int Clear(SequentialContainer *gen)
{
	return gen->vTable->Clear(gen);
}

static int Contains(const SequentialContainer *gen,const void *value)
{
	return gen->vTable->Contains(gen,value);
}

static int Erase(SequentialContainer *gen,const void *elem)
{
	return gen->vTable->Erase(gen,elem);
}

static int EraseAll(SequentialContainer *gen,const void *elem)
{
        return gen->vTable->EraseAll(gen,elem);
}

static int Finalize(SequentialContainer *gen)
{
	return gen->vTable->Finalize(gen);
}

static void Apply(SequentialContainer *Gen,int (*Applyfn)(void *,void * arg),void *arg)
{
	Gen->vTable->Apply(Gen,Applyfn,arg);
}

static int Equal(const SequentialContainer *Gen1,const SequentialContainer *Gen2)
{
	return Gen1->vTable->Equal(Gen1,Gen2);
}

static SequentialContainer *Copy(const SequentialContainer *src)
{
	return src->vTable->Copy(src);
}

static ErrorFunction SetErrorFunction(SequentialContainer *Gen,ErrorFunction fn)
{
	return Gen->vTable->SetErrorFunction(Gen,fn);
}

static size_t Sizeof(const SequentialContainer *gen)
{
	return gen->vTable->Sizeof(gen);
}

static Iterator *NewIterator(SequentialContainer *gen)
{
	return gen->vTable->NewIterator(gen);
}
static int InitIterator(SequentialContainer *gen,void *buf)
{
        return gen->vTable->InitIterator(gen,buf);
}

static int deleteIterator(Iterator *git)
{
	SequentialIterator *GenIt = (SequentialIterator *)git;
	SequentialContainer *gen = GenIt->Gen;
	return gen->vTable->deleteIterator(git);
}

static size_t SizeofIterator(const SequentialContainer *git)
{
	SequentialIterator *GenIt = (SequentialIterator *)git;
        SequentialContainer *gen = GenIt->Gen;
        return gen->vTable->SizeofIterator(git);
}

static int Save(const SequentialContainer *gen, FILE *stream,SaveFunction saveFn,void *arg)
{
	return gen->vTable->Save(gen,stream,saveFn,arg);
}
/*-----------------------------------------------------------------------------------*/
/*                                                                                   */
/*                             Sequential containers                                 */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/

static int Add(SequentialContainer *sc, const void *Element)
{
	return sc->vTable->Add(sc,Element);
}

static void *GetElement(const SequentialContainer *sc,size_t idx)
{
	return sc->vTable->GetElement(sc,idx);
}

static int Push(SequentialContainer *gen,void *Element)
{
	return gen->vTable->Push(gen,Element);
}

static int Pop(SequentialContainer *g,void *Element)
{
	return g->vTable->Pop(g,Element);
}

static int InsertAt(SequentialContainer *gen,size_t idx,const void *newval)
{
	return gen->vTable->InsertAt(gen,idx,newval);
}

static int EraseAt(SequentialContainer *g,size_t idx)
{
	return g->vTable->EraseAt(g,idx);
}

static int ReplaceAt(SequentialContainer *s,size_t idx,const void *newelem)
{
	return s->vTable->ReplaceAt(s,idx,newelem);
}

static int IndexOf(const SequentialContainer *g,const void *elemToFind,
                   void *args,size_t *result)
{
	return g->vTable->IndexOf(g,elemToFind,args,result);
}


static int Append(SequentialContainer *g1,SequentialContainer *g2)
{
	int r;
	void *element;
	Iterator *it1 = NewIterator((SequentialContainer *)
		g2);
	for (element = it1->GetFirst(it1);
                   element != NULL;
                   element = it1->GetNext(it1)) {
		r = Add(g1,element);
		if (r <=0) {
			return r;
		}
		
	}
	return 1;
}
	

SequentialContainerInterface iSequentialContainer = {
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
Add,
GetElement,
Push,
Pop,
InsertAt,
EraseAt,
ReplaceAt,
IndexOf,
Append,
};
