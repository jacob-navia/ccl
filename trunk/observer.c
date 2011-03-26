#include "containers.h"
typedef struct _tagObserver {
	void *ObservedObject;
	ObserverFunction Callback;
	unsigned Flags;
} Observer;

static Vector *ObserverVector;

static int initVector(void)
{
	ObserverVector = iVector.Create(sizeof(Observer),25);
	if (ObserverVector == NULL) {
		iError.RaiseError("iObserver.Subscribe",CONTAINER_ERROR_NOMEMORY);
		return 0;
	}
	return 1;
}

static int InitObserver(Observer *result,void *ObservedObject, ObserverFunction callback, unsigned flags)
{
	GenericContainer *gen = ObservedObject;
	unsigned Subjectflags = iGeneric.GetFlags(gen);
	Subjectflags |= CONTAINER_HAS_OBSERVER;
	iGeneric.SetFlags(gen,Subjectflags);
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
		r = iVector.Add(ObserverVector,&result);
	return r;
}

static int Notify(void *ObservedObject,unsigned operation,void *ExtraInfo)
{
	int r,count;
	size_t idx = 0;
	Observer obs,*pObserver;
	memset(&obs,0,sizeof(obs));
	obs.ObservedObject = ObservedObject;
	r = iVector.SearchWithKey(ObserverVector,0,sizeof(void *),idx,&obs,&idx);
	if (r <= 0)
		return 0;
	do {
		pObserver = iVector.GetElement(ObserverVector,idx);
		if (pObserver->Flags & operation) {
			pObserver->Callback(ObservedObject,operation,ExtraInfo);	
			count++;
		}
		idx++;
		r = iVector.SearchWithKey(ObserverVector,0,sizeof(void *),idx,&obs,&idx);
	} while (r > 0);
	return 0;
}

static size_t Unsubscribe(void *ObservedObject,ObserverFunction callback)
{
	Observer *pObserver;
	size_t idx=0,count=0;
	int r;

	if (ObservedObject == NULL) {
		/* Erase all observers that have the specified function. This means that the
		  object receiving the callback goes out of scope */
		if (callback == NULL) /* If both are NULL do nothing */
			return 0;
		/* Search for the callback */
		r = iVector.SearchWithKey(ObserverVector,offsetof(Observer,Callback),sizeof(void *),idx,&callback,&idx);
		if (r <= 0) /* If not found do nothing */
			return r;
		do {
			/* Erase all observers with given callback */
			pObserver = iVector.GetElement(ObserverVector,idx);
			iVector.Erase(ObserverVector,pObserver);
			count++;
			r = iVector.SearchWithKey(ObserverVector,0,sizeof(void *),idx,&callback,&idx);
		} while (r > 0);
		return count;
	}
	if (callback == NULL) {
		r = iVector.SearchWithKey(ObserverVector,offsetof(Observer,ObservedObject),sizeof(void *),idx,&callback,&idx);
		if (r <= 0)
			return r;
		do {
			pObserver = iVector.GetElement(ObserverVector,idx);
			iVector.Erase(ObserverVector,pObserver);
			r = iVector.SearchWithKey(ObserverVector,0,sizeof(void *),idx,&callback,&idx);
			count++;
		} while (r > 0);
		return count;
	}
	r = iVector.SearchWithKey(ObserverVector,offsetof(Observer,ObservedObject),sizeof(void *),idx,&callback,&idx);
	if (r<=0)
		return r;
	do {
	/* Erase all observers with given callback AND given observed object */
		pObserver = iVector.GetElement(ObserverVector,idx);
		if (pObserver->ObservedObject == ObservedObject) {
			iVector.Erase(ObserverVector,pObserver);
			count++;
		}
		r = iVector.SearchWithKey(ObserverVector,0,sizeof(void *),idx,&callback,&idx);
	} while (r > 0);
	return count;
}

ObserverInterface iObserver = {
	Subscribe,
	Notify,
	Unsubscribe
};
