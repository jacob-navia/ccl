#include "containers.h"
#include "ccl_internal.h"
struct GenericContainer {
	GenericContainerInterface *vTable;
	size_t Size;
	unsigned Flags;
	size_t ElementSize;
};

struct SequentialContainer {
	SequentialContainerInterface *vTable;
	size_t Size;
	unsigned Flags;
	size_t ElementSize;
};

static size_t Size(GenericContainer *gen)
{
	if (gen == NULL) {
		iError.RaiseError("iGeneric.Size",CONTAINER_ERROR_BADARG);
		return 0;
	}
	return gen->Size;
}

static unsigned GetFlags(GenericContainer  *gen)
{
	if (gen == NULL) {
		iError.RaiseError("iGneric.GetFlags",CONTAINER_ERROR_BADARG);
		return 0;
	}
	return gen->Flags;
}

static unsigned SetFlags(GenericContainer *gen,unsigned newFlags)
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

static int Clear(GenericContainer *gen)
{
	return gen->vTable->Clear(gen);
}

static int Contains(GenericContainer *gen,void *value)
{
	return gen->vTable->Contains(gen,value);
}

static int Erase(GenericContainer *gen,void *elem)
{
	return gen->vTable->Erase(gen,elem);
}

static int Finalize(GenericContainer *gen)
{
	return gen->vTable->Finalize(gen);
}

static void Apply(GenericContainer *Gen,int (*Applyfn)(void *,void * arg),void *arg)
{
	Gen->vTable->Apply(Gen,Applyfn,arg);
}

static int Equal(GenericContainer *Gen1,GenericContainer *Gen2)
{
	return Gen1->vTable->Equal(Gen1,Gen2);
}

static GenericContainer *Copy(GenericContainer *src)
{
	return src->vTable->Copy(src);
}

static ErrorFunction SetErrorFunction(GenericContainer *Gen,ErrorFunction fn)
{
	return Gen->vTable->SetErrorFunction(Gen,fn);
}

static size_t Sizeof(GenericContainer *gen)
{
	return gen->vTable->Sizeof(gen);
}

typedef struct tagGenericIterator {
	Iterator it;
	GenericContainer *Gen;
} GenericIterator;

static Iterator *newIterator(GenericContainer *gen)
{
	return gen->vTable->newIterator(gen);
}
static int deleteIterator(Iterator *git)
{
	GenericIterator *GenIt = (GenericIterator *)git;
	GenericContainer *gen = GenIt->Gen;
	return gen->vTable->deleteIterator(git);
}

static int Save(GenericContainer *gen, FILE *stream,SaveFunction saveFn,void *arg)
{
	return gen->vTable->Save(gen,stream,saveFn,arg);
}
/*-----------------------------------------------------------------------------------*/
/*                                                                                   */
/*                             Sequential containers                                 */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/

static int Add(SequentialContainer *sc, void *Element)
{
	return sc->vTable->Add(sc,Element);
}

static void *GetElement(SequentialContainer *sc,size_t idx)
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

static int InsertAt(SequentialContainer *gen,size_t idx,void *newval)
{
	return gen->vTable->InsertAt(gen,idx,newval);
}

static int EraseAt(SequentialContainer *g,size_t idx)
{
	return g->vTable->EraseAt(g,idx);
}

static int ReplaceAt(SequentialContainer *s,size_t idx,void *newelem)
{
	return s->vTable->ReplaceAt(s,idx,newelem);
}

static int IndexOf(SequentialContainer *g,void *elemToFind,void *args,size_t *result)
{
	return g->vTable->IndexOf(g,elemToFind,args,result);
}


static int Append(SequentialContainer *g1,SequentialContainer *g2)
{
	int r;
	void *element;
	Iterator *it1 = newIterator((GenericContainer *)
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
	

GenericContainerInterface iGeneric = {
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
};

SequentialContainerInterface iSequentialContainer = {
{
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
},
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
