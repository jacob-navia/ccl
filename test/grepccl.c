#include "../containers.h"

static void grep(char *word,char *fileName)
{
    strCollection *text = istrCollection.CreateFromFile(fileName);
	Iterator *it;
	int lineNumber = 1;
	char *line;
	if (text == NULL) {
		fprintf(stderr,"Unable to open '%s'\n",fileName);
		return;
	}
	it = istrCollection.NewIterator(text);
	for (line = it->GetFirst(it); line!= NULL; 
    		line = it->GetNext(it),lineNumber++) {
		char *p = strstr(line,word);
		if (p) {
			printf("[%4d] %s\n",lineNumber,line);
		}
	}
	istrCollection.DeleteIterator(it);
	istrCollection.Finalize(text);
}

int main(int argc,char *argv[])
{
	if (argc < 3) {
		fprintf(stderr,"Usage: grep <word> <file>\n");
		return EXIT_FAILURE;
	}
	grep(argv[1],argv[2]);
}
