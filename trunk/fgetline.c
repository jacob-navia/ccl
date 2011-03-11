#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "containers.h"
/* This code was adapted from the public domain version of fgetline of C.B. Falconer */

static int GetDelim(unsigned char **LinePointer, int *n, int delimiter, FILE *stream, ContainerMemoryManager *mm )
{
	unsigned char *p,*newp;
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

int GetLine(unsigned char **LinePointer,int *n, FILE *stream,ContainerMemoryManager *mm)
{
	return GetDelim(LinePointer,n,'\n',stream,mm);
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
