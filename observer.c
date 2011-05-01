#include "containers.h"
#include "ccl_internal.h"
typedef struct _tagObserver {
	void *ObservedObject; /* The object being observed */
	ObserverFunction Callback; /* The function to call */
	unsigned Flags; /* The events the observer wishes to be informed about */
} Observer;

static Observer *ObserverVector;
static size_t vsize;
#define CHUNK_SIZE	25

static int initVector(void)
{
	ObserverVector = calloc(sizeof(Observer),CHUNK_SIZE);
	if (ObserverVector == NULL) {
		iError.RaiseError("iObserver.Subscribe",CONTAINER_ERROR_NOMEMORY);
		return 0;
	}
	vsize=CHUNK_SIZE;
	return 1;
}

static int  AddObject(Observer *ob)
{
	size_t i;
	Observer *tmp;
	

	for (i=0; i<vsize;i++) {
		if (ObserverVector[i].ObservedObject==NULL) {
			memcpy(ObserverVector+i,ob,sizeof(Observer));
			return 1;
		}
	}
	tmp = realloc(ObserverVector,(vsize+CHUNK_SIZE)*sizeof(Observer));
	if (tmp == NULL) {
		iError.RaiseError("iObserver.Subscribe",CONTAINER_ERROR_NOMEMORY);
		return CONTAINER_ERROR_NOMEMORY;
	}
	ObserverVector = tmp;
	memset(ObserverVector+vsize+1,0,(CHUNK_SIZE-1)*sizeof(Observer));
	memcpy(ObserverVector+vsize,ob,sizeof(Observer));
	vsize+= CHUNK_SIZE;
	return 1;
}

static int InitObserver(Observer *result,void *ObservedObject, ObserverFunction callback, unsigned flags)
{
	GenericContainer *gen = ObservedObject;
	unsigned Subjectflags = gen->Flags;
	Subjectflags |= CONTAINER_HAS_OBSERVER;
	gen->Flags=Subjectflags;
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
		r = AddObject(&result);
	return r;
}


static int Notify(void *ObservedObject,unsigned operation,void *ExtraInfo1,void *ExtraInfo2)
{
	int count=0;
	size_t idx = 0;
	void *ExtraInfo[2];

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
