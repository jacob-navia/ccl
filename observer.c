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

static int Subscribe(void *ObservedObject, ObserverFunction callback, unsigned flags)
{
        size_t i;
        Observer *pObs=NULL;
        GenericContainer *gen = ObservedObject;
        unsigned Subjectflags = gen->Flags;

        Subjectflags |= CONTAINER_HAS_OBSERVER;
        gen->Flags=Subjectflags;
        if (ObserverVector == NULL) {
		ObserverVector = calloc(sizeof(Observer),CHUNK_SIZE);
		if (ObserverVector == NULL) {
			iError.RaiseError("iObserver.Subscribe",CONTAINER_ERROR_NOMEMORY);
	                return CONTAINER_ERROR_NOMEMORY;
		}
		vsize = CHUNK_SIZE;
        }
        for (i=0; i<vsize;i++) {
            if (ObserverVector[i].ObservedObject==NULL) {
				pObs = ObserverVector+i;
				break;
            }
        }
	if (i >= vsize) {
        	Observer *tmp = realloc(ObserverVector,(vsize+CHUNK_SIZE)*sizeof(Observer));
        	if (tmp == NULL) {
               		iError.RaiseError("iObserver.Subscribe",CONTAINER_ERROR_NOMEMORY);
                	return CONTAINER_ERROR_NOMEMORY;
		}
        	ObserverVector = tmp;
        	memset(ObserverVector+vsize+1,0,(CHUNK_SIZE-1)*sizeof(Observer));
		pObs = ObserverVector + vsize;
        	vsize+= CHUNK_SIZE;
	}
	pObs->ObservedObject = ObservedObject;
	pObs->Callback = callback;
	pObs->Flags = flags;
	return 1;
}


static int Notify(const void *ObservedObject,unsigned operation,const void *ExtraInfo1,const void *ExtraInfo2)
{
	int count=0;
	size_t idx;
	const void *ExtraInfo[2];

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
	size_t idx=0,count=0;

	if (ObservedObject == NULL) {
		/* Erase all observers that have the specified function. This means that the
		  object receiving the callback goes out of scope */
		if (callback) /* If both are NULL do nothing */
			for (; idx<vsize;idx++) {
				if (ObserverVector[idx].Callback == callback) {
					memset(ObserverVector+idx,0,sizeof(Observer));
					count++;
				}
		}
	}
	else if (callback == NULL) {
		for (;idx<vsize;idx++) {
			if (ObserverVector[idx].ObservedObject == ObservedObject) {
				memset(ObserverVector+idx,0,sizeof(Observer));
				count++;
			}
		}
	}
	else for (; idx<vsize;idx++) {
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
