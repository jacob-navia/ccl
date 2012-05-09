#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "containers.h"
#include "ccl_internal.h"
/* This code was adapted from the public domain version of fgetline by C.B. Falconer */

static int GetDelim(char **LinePointer, int *n, int delimiter, FILE *stream, ContainerAllocator *mm )
{
	char *p,*newp;
	size_t d;
	int c;
	int len = 0;

	if (stream == NULL || LinePointer == NULL || n == NULL || 
		(*LinePointer && *n == 0)) {
		iError.RaiseError("GetLine",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}

	if (!*LinePointer || !*n) {
		p = mm->realloc(*LinePointer, BUFSIZ );
		if (!p) {
			iError.RaiseError("GetLine",CONTAINER_ERROR_NOMEMORY);
			return CONTAINER_ERROR_NOMEMORY;
		}
		*n = BUFSIZ;
		*LinePointer = p;
	}

	else p = *LinePointer;

	/* read until delimiter or EOF */
	while ((c = fgetc( stream )) != EOF) {
		if (len >= *n) {
			d = p - *LinePointer;
			newp = mm->realloc(*LinePointer, *n * 2 );
			if (!newp) 
				goto NoMem;
			p = newp + d;
			*LinePointer = newp;
			*n *= 2;
		}
		if (delimiter == c)
			break;
		*p++ = (char) c;
		len++;
		if (len == INT_MAX-1)
			break;
	}

	/* Look for EOF without any bytes read condition */
	if ((c == EOF) && (len == 0))
		return EOF;

	if (len >= *n) {
		c = (int)(p - *LinePointer);
		newp = mm->realloc( *LinePointer, *n + 1 );
		if (!newp) {
NoMem:
			mm->free(*LinePointer);
			*LinePointer = NULL;
			iError.RaiseError("Getline",CONTAINER_ERROR_NOMEMORY);
			return CONTAINER_ERROR_NOMEMORY;
		}
		p = newp + c;
		*LinePointer = newp;
		*n += 1;
	}
	*p = 0;
	return len;
}

int GetLine(char **LinePointer,int *n, FILE *stream,ContainerAllocator *mm)
{
	return GetDelim(LinePointer,n,'\n',stream,mm);
}

static int WGetDelim(wchar_t **LinePointer, int *n, int delimiter, FILE *stream, ContainerAllocator *mm )
{
	wchar_t *p,*newp;
	size_t d;
	int c;
	int len = 0;
	
	if (stream == NULL || LinePointer == NULL || n == NULL || 
		(*LinePointer && *n == 0)) {
		iError.RaiseError("GetLine",CONTAINER_ERROR_BADARG);
		return CONTAINER_ERROR_BADARG;
	}
	
	if (!*LinePointer || !*n) {
		p = mm->realloc(*LinePointer, BUFSIZ*sizeof(wchar_t) );
		if (!p) {
			iError.RaiseError("GetLine",CONTAINER_ERROR_NOMEMORY);
			return CONTAINER_ERROR_NOMEMORY;
		}
		*n = BUFSIZ;
		*LinePointer = p;
	}
	
	else p = *LinePointer;
	
	/* read until delimiter or EOF */
	while ((c = getwc( stream )) != EOF) {
		if (len >= *n) {
			d = p - *LinePointer;
			newp = mm->realloc(*LinePointer, *n * 2 *sizeof(wchar_t) );
			if (!newp) 
				goto NoMem;
			p = newp + d;
			*LinePointer = newp;
			*n *= 2;
		}
		if (delimiter == c)
			break;
		*p++ = (wchar_t) c;
		len++;
		if (len == INT_MAX-1)
			break;
	}
	
	/* Look for EOF without any bytes read condition */
	if ((c == EOF) && (len == 0))
		return EOF;
	
	if (len >= *n) {
		c = (int)(p - *LinePointer);
		newp = mm->realloc( *LinePointer, *n + 1 );
		if (!newp) {
		NoMem:
			mm->free(*LinePointer);
			*LinePointer = NULL;
			iError.RaiseError("Getline",CONTAINER_ERROR_NOMEMORY);
			return CONTAINER_ERROR_NOMEMORY;
		}
		p = newp + c;
		*LinePointer = newp;
		*n += 1;
	}
	*p = 0;
	return len;
}

int WGetLine(wchar_t **LinePointer,int *n, FILE *stream,ContainerAllocator *mm)
{
	return WGetDelim(LinePointer,n,L'\n',stream,mm);
}

#ifdef TEST
int main(void)
{
	char *buf=NULL;
	int n=0;
	int r = GetLine(&buf,&n,stdin);
	printf("'%s'\n",buf);
}
#endif
