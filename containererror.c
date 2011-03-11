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
static void ContainerRaiseError(const char *fnname,int errcode,...)
{
	fprintf(stderr,"Container library: Error '%s' in function %s\n",StrError(errcode),fnname);
}
static void EmptyErrorFunction(const char *fnname,int errcode,...) { }

static ErrorFunction SetError(ErrorFunction n)
{
	ErrorFunction old = iError.RaiseError;
	if (n != NULL) {
		iError.RaiseError = n;
	}
	return old;
}

static int LibraryError(const char *interfaceName,const char *fnname, int errorCode)
{
	char buf[256];

	if (strlen(fnname)+strlen(interfaceName)+10 < sizeof(buf))
		sprintf(buf,"%s.%s",interfaceName,fnname);
	else {
		size_t nameLen = strlen(interfaceName);
		size_t fnnameLen = strlen(fnname);

		if (nameLen > sizeof(buf)/3) {
			memcpy(buf,interfaceName,sizeof(buf)/3);
			buf[sizeof(buf)/3] = 0;
		}
		else strcpy(buf,interfaceName);
		strcat(buf,".");
		nameLen = strlen(buf);
		if (fnnameLen > sizeof(buf)/3) {
			memcpy(&buf[nameLen],fnname,sizeof(buf)/3);
			buf[fnnameLen+sizeof(buf)/3]=0;
		}
		else strcat(buf,fnname);
	}
	ContainerRaiseError(buf,errorCode);
	return errorCode;
}


ErrorInterface iError = {
	ContainerRaiseError,
	EmptyErrorFunction,
	StrError,
	SetError,
	LibraryError,
};


/* Decode ULE128 string */

int decode_ule128(FILE *stream, size_t *val)
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

int encode_ule128(FILE *stream,size_t val)
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

