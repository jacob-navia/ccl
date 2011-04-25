#include "containers.h"
typedef struct _tagObserver {
	void *ObservedObject;
	ObserverFunction Callback;
	unsigned Flags;
} Observer;

static Observer *ObserverVector;
static size_t vsize;

static int initVector(void)
{
	ObserverVector = calloc(sizeof(Observer),25);
	if (ObserverVector == NULL) {
		iError.RaiseError("iObserver.Subscribe",CONTAINER_ERROR_NOMEMORY);
		return 0;
	}
	vsize=25;
	return 1;
}

static int  OAdd(Observer *ob)
{
	size_t i;
	Observer *tmp;
	

	for (i=0; i<vsize;i++) {
		if (ObserverVector[i].ObservedObject==NULL) {
			memcpy(ObserverVector+i,ob,sizeof(Observer));
			return 1;
		}
	}
	tmp = realloc(ObserverVector,(vsize+25)*sizeof(Observer));
	if (tmp == NULL) {
		iError.RaiseError("iObserver.Subscribe",CONTAINER_ERROR_NOMEMORY);
		return CONTAINER_ERROR_NOMEMORY;
	}
	ObserverVector = tmp;
	memset(ObserverVector+vsize+1,0,(25-1)*sizeof(Observer));
	memcpy(ObserverVector+vsize,ob,sizeof(Observer));
	vsize+= 25;
	return 1;
}

static int InitObserver(Observer *result,void *ObservedObject, ObserverFunction callback, unsigned flags)
{
	GenericContainer *gen = ObservedObject;
	unsigned Subjectflags = gen->vTable->GetFlags(gen);
	Subjectflags |= CONTAINER_HAS_OBSERVER;
	gen->vTable->SetFlags(gen,Subjectflags);
	memset(result,0,sizeof(Observer));
	result->ObservedObject = ObservedObject;
	result->Callback = callback;
	result->Flags = flags;
	if (ObserverVector == NULL && initVector() == 0) {
		return CONTAINER_ERROR_NOMEMORY;
	}
	return 1;
}

static int Subscribe(void *ObservedObject, ObserverFunction callback, unsigned flags)
{
	Observer result;
	int r;
	r = InitObserver(&result,ObservedObject,callback,flags);
	if (r > 0)
		r = OAdd(&result);
	return r;
}


static int Notify(void *ObservedObject,unsigned operation,void *ExtraInfo1,void *ExtraInfo2)
{
	int count=0;
	size_t idx = 0;
	void *ExtraInfo[2];
	Observer obs;

	memset(&obs,0,sizeof(obs));
	obs.ObservedObject = ObservedObject;
	ExtraInfo[0] = ExtraInfo1;
	ExtraInfo[1] = ExtraInfo2;
	for (idx=0; idx < vsize;idx++) {
		if (ObserverVector[idx].ObservedObject == ObservedObject) {
			if (ObserverVector[idx].Flags & operation) {
				ObserverVector[idx].Callback(ObservedObject,operation,ExtraInfo);
				count++;
			}
		}
	}
	return count;
}

static size_t Unsubscribe(void *ObservedObject,ObserverFunction callback)
{
	size_t idx,count=0;

	if (ObservedObject == NULL) {
		/* Erase all observers that have the specified function. This means that the
		  object receiving the callback goes out of scope */
		if (callback == NULL) /* If both are NULL do nothing */
			return 0;
		for (idx=0; idx<vsize;idx++) {
			if (ObserverVector[idx].Callback == callback) {
				memset(ObserverVector+idx,0,sizeof(Observer));
				count++;
			}
		}
		return count;
	}
	if (callback == NULL) {
		for (idx=0;idx<vsize;idx++) {
			if (ObserverVector[idx].ObservedObject == ObservedObject) {
				memset(ObserverVector+idx,0,sizeof(Observer));
				count++;
			}
		}
		return count;
	}
	for (idx=0; idx<vsize;idx++) {
		if (ObserverVector[idx].ObservedObject == ObservedObject &&
			ObserverVector[idx].Callback == callback) {
			memset(ObserverVector+idx,0,sizeof(Observer));
			count++;
		}
	}
	return count;
}

ObserverInterface iObserver = {
	Subscribe,
	Notify,
	Unsubscribe
};
