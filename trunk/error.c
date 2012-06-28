#include "containers.h"
#include <stdio.h>
struct Error {
	int code;
	char *Message;
} ErrorMessages[] = {
	{CONTAINER_ERROR_BADARG,        "Bad argument"},
	{CONTAINER_ERROR_NOMEMORY,      "Insufficient memory"},
	{CONTAINER_ERROR_INDEX,         "Index error"},
	{CONTAINER_ERROR_READONLY,      "Object is read only"},
	{CONTAINER_INTERNAL_ERROR,      "Internal error in container library"},
	{CONTAINER_ERROR_OBJECT_CHANGED,"Iterator used with modified object"},
	{CONTAINER_ERROR_NOT_EMPTY,     "container is not empty"},
	{CONTAINER_ERROR_FILE_READ,     "read error in input stream"},
	{CONTAINER_ERROR_FILE_WRITE,    "write error in output stream"},
	{CONTAINER_FULL,				"Container is full"},
	{CONTAINER_ASSERTION_FAILED,	"Assertion failed"},
	{CONTAINER_ERROR_NOENT,         "File not found"},
	{CONTAINER_ERROR_FILEOPEN,		"Error opening the file"},
	{CONTAINER_ERROR_INCOMPATIBLE,  "Incompatible element sizes"},
	{CONTAINER_ERROR_WRONGFILE,		"Not a container stream"},
	{CONTAINER_ERROR_NOTIMPLEMENTED,"Function not implemented for this container type"},
	{CONTAINER_ERROR_BADPOINTER,    "Debug_malloc: BAD POINTER******"},
	{CONTAINER_ERROR_BUFFEROVERFLOW,"Debug_malloc: BUFFER OVERFLOW******"},
	{CONTAINER_ERROR_WRONGELEMENT,  "Wrong element passed to a list"},
	{CONTAINER_ERROR_BADMASK,       "Incorrect mask length"},
	{0,"Unknown error"},
};

static char *StrError(int errcode)
{
	struct Error *e = ErrorMessages;
	while (e->code) {
		if (e->code == errcode)
			break;
		e++;
	}
	return e->Message;
}
static void *ContainerRaiseError(const char *fnname,int errcode,...)
{
	fprintf(stderr,"Container library: Error '%s' in function %s\n",StrError(errcode),fnname);
	return NULL;
}
static void *EmptyErrorFunction(const char *fnname,int errcode,...) { return NULL; }

static ErrorFunction SetError(ErrorFunction n)
{
	ErrorFunction old = iError.RaiseError;
	if (n != NULL) {
		iError.RaiseError = n;
	}
	return old;
}

static int BadArgError(const char *fname)
{
	iError.RaiseError(fname,CONTAINER_ERROR_BADARG);
	return CONTAINER_ERROR_BADARG;
}


ErrorInterface iError = {
	ContainerRaiseError,
	EmptyErrorFunction,
	StrError,
	SetError,
	BadArgError,
};

