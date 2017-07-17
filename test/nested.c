#include <ctype.h>
#include "../containers.h"

static char *GetWord(char *line)
{
	if (line == NULL || *line == 0) return NULL;
	if (!(isalpha(*line) || isdigit(*line) || *line == '_')) 
		return line+1;
	while (isalpha(*line) || isdigit(*line) || *line == '_')
		line++;
	return line;
}

int main(int argc,char *argv[])
{
	Dictionary *dict = iDictionary.Create(sizeof(ValArrayInt *),1000);
	char *start,*end,*word,*line;
	strCollection *text = istrCollection.CreateFromFile(argv[1]);
	ValArrayInt *positions;
	size_t top,pos;
	int c,i,j;

	top = istrCollection.Size(text);
	for (i=0; i<top;i++) {
		line = istrCollection.GetElement(text,i);
		start = line;
		while (isspace(*start)) start++;
		end = GetWord(start);
		while (end) {
			c = *end;
			*end = 0;
			if (! iDictionary.Contains(dict,start)) {
				positions = iValArrayInt.Create(10);
				iDictionary.Add(dict,start,&positions);
			}
			else iDictionary.CopyElement(dict,start,&positions);
			iValArrayInt.Add(positions,i);
			*end = c;
			start = end;
			while (isspace(*start)) start++;
			end = GetWord(start);
		}
	}
	strCollection *Words = iDictionary.GetKeys(dict);
	top = istrCollection.Size(Words);
	for (i=0; i<top;i++)  { 
		word = istrCollection.GetElement(Words,i);
		printf("'%s':  ",word);
		iDictionary.CopyElement(dict,word,&positions);
		pos = iValArrayInt.Size(positions);
		for (j=0; j<pos;j++) {
			printf("%d ",1+iValArrayInt.GetElement(positions,j));
		}
		printf("\n");
		iValArrayInt.Finalize(positions);
	}
	istrCollection.Finalize(text);
	istrCollection.Finalize(Words);
	iDictionary.Finalize(dict);
}
