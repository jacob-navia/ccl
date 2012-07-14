#include "containers.h"
static char *ContainerNames[] ={
	"List",
	"Dlist",
//	"EXTERNAL_NAME",
	"Vector",
	"ValArray",
	"BitString",
	"strCollection",
	"Queue",
	"Deque",
	"Dictionary",
	"HashTable",
	"TreeMap",
	"PQueue",
	"StreamBuffer",
	NULL
};

static char *ExceptionsTable[] = {
	"Create",
	"CreateWithAllocator",
	"CreateFromFile",
	"CreateSequence",
	"InitializeWith",
	"Init",
	"InitWithAllocator",
	"Load",
	"StringToBitString",
	"ObjectToBitString",
};

#define NbContainers (sizeof(ContainerNames)/sizeof(ContainerNames[0])-1)

typedef struct Data {
	unsigned char *ApiName;
	int type[NbContainers];
} DataPoint;

int Strtrim(char *str)
{
        char *src = str,*dst = str,*start = str;

        while (isspace(*src))
                src++;
        do {
                while (*src && !isspace(*src))
                        *dst++ = *src++;
                if (*src) {
                        *dst++ = *src++;
                        while (isspace(*src) && *src != '\n' && *src != '\r')
                                src++;
                }
        } while (*src);
        if (dst != start && isspace(dst[-1]) && dst[-1] != '\n')
                dst--;
        *dst = 0;
        return dst - src;
}


static Dictionary *Data;
static strCollection *Exceptions;
static strCollection *ContainerTable;

static int FindType(char *type)
{
	size_t typ=0;
	int r = istrCollection.IndexOf(ContainerTable,type,&typ);
	if (r < 0)
		return r;
	return typ;
}

static int isId(char *p)
{
	if (*p >= 'a' && *p <= 'z')
		return 1;
	if (*p >= 'A' && *p <= 'Z')
		return 1;
	if (*p >= '0' && *p <='9')
		return 1;
	if (*p == '_')
		return 1;
	return 0;
}

static char *GetId(char *start)
{
	while (isId(start))
		start++;
	return start;
}

static int ScanOneLine(unsigned char *line)
{
	char *q,*p = strchr(line,')');
	DataPoint d;
	const DataPoint *old=NULL;
	size_t typ=0;
	int t;
	if (p == NULL)
		return -1;
	q = p+1;
	*p-- = 0;
	while (*p == ' ' )
		p--;
	while (isId(p)) {
		p--;
	}
	p++;
	while (*q == ' ')  q++;
	if (*q == '(')
		q++;
	else return -1;


	if (Data == NULL) {
		Data = iDictionary.Create(sizeof(DataPoint),200);
	}
	old = iDictionary.GetElement(Data,p);

	memset(&d,0,sizeof(d));
	t = istrCollection.IndexOf(Exceptions,p,&typ);
	//printf("%s: t=%d, index %ull\n",p,t,typ);
	if (t >= 0) {
		char *save = p;
		p = line;

		while (*p == ' ' || *p == '\t')
			p++;
		q = GetId(p);
		*q = 0;
		t = FindType(p);
		if (t < 0) {
			if (strcmp(p,"ElementType")) {
				return -1;
			}
			typ = 3;
		}
		else typ = t;
		if (old == NULL) d.ApiName = strdup(save);
	}
	else {
		if (old ==NULL) d.ApiName = strdup(p);
		p = q;
		q = GetId(q);
		*q=0;
		if (!strcmp(p,"const")) {
			q++;
			while (*q == ' ' || *q == '\t')
				q++;
			p = q;
			q = GetId(q);
			*q=0;
		}
		t = FindType(p);
		if (t < 0) {
			if (strcmp(p,"ElementType")) {
				free(d.ApiName);
				return -1;
			}
			typ = 12;
		}
		else typ = t;
	}
	if (old) {
		memcpy(&d,old,sizeof(DataPoint));
	}	
	d.type[typ] = typ+1;
	if (Data == NULL) {
		Data = iDictionary.Create(sizeof(DataPoint),200);
	}

	iDictionary.Add(Data,d.ApiName,&d);
	return 1;
}

static int comp(const void **s1,const void **s2, CompareInfo *info)
{
	const char *p = strstr(*s1,"(*");
	const char *q = strstr(*s2,"(*");
	if (p == NULL || q == NULL) {
		fprintf(stderr,"BUG***\n");
		exit(-1);
	}
	return strcmp(p,q);
}

static void EliminateComments(strCollection *sc)
{
	Iterator *it = istrCollection.NewIterator(sc);
	char *p;

	for (p = it->GetFirst(it); p; p = it->GetNext(it)) {
		char *q = strstr(p,"/*");
		if (q) *q = 0;
	}
}

static int trim(strCollection *sc,int idx,char *p)
{
	char *q  = strrchr(p,',');
	char *s = strchr(p,'(');
	char buf[1024];
	int i,len1,save;

	if (q == NULL) return 1;
	if (s == NULL) s = p;
	while (q-p > 70) {
		char *tmp;
		*q=0;
		tmp = strrchr(p,',');
		*q=',';
		if (tmp != NULL)
			q=tmp;
		else break;
	}
	len1 = s-p;
	if (len1+strlen(q) > 75) {
		len1 = 75-strlen(q);
	}
	if (len1 < 8) len1 = 8;
	for (i=0; i<len1;i++)
		buf[i]=' ';
	buf[i]=0;
	strcat(buf,q+1);
	q++;
	*q=0;
	istrCollection.InsertAt(sc,idx+1,buf);
	if (strlen(buf) > 75) {
		return 1+trim(sc,idx+1,istrCollection.GetElement(sc,idx+1));
	}
	return 2;
}
	

static void doFile(char *name,Iterator *it)
{
	FILE *f;
	const char *p;
	char *q,buf[4096];
	int counter = 0;
	Iterator *It;
	strCollection *sc = istrCollection.CreateWithAllocator(50,&iDebugMalloc);
	do {
		p = it->GetNext(it);
		q = strstr(p,"//");
		if (q) *q=0;
		q = strstr(p,"(*");
		if (q) {
			Strtrim(p);
			sprintf(buf,"   %s",p);
			istrCollection.Add(sc,buf);
		}
	} while (strchr(p,'}') == 0);
	istrCollection.SetCompareFunction(sc,comp);
	EliminateComments(sc);
	istrCollection.Sort(sc);
	istrCollection.InsertAt(sc,0,"\\begin{verbatim}");
	sprintf(buf,"typedef struct tag%sInterface {",name);
	istrCollection.InsertAt(sc,1,buf);
	sprintf(buf,"} %sInterface;",name);
	istrCollection.Add(sc,buf);
	istrCollection.Add(sc,"\\end{verbatim}");
	while (counter < istrCollection.Size(sc)) {
		p = istrCollection.GetElement(sc,counter);
		if (strlen(p) > 70) counter += trim(sc,counter,p);
		else counter++;
	}
	strcat(name,".tex");
	istrCollection.WriteToFile(sc,name);
}


static void ExtractIds(char *fname)
{
	strCollection *text = istrCollection.CreateFromFile(fname);
	Iterator *it = istrCollection.NewIterator(text);
	char *p;
	for (p = it->GetFirst(it); p != NULL; p = it->GetNext(it)) {
		char buf[512];
		if (strstr(p,"typedef") && strstr(p,"struct") &&
                    strchr(p,'{')) {
			char *q = strstr(p,"struct");
			char *name;
			q += 6;
			while (*q == ' ' || *q == '\t')
				q++;
			if (*q == 't' && q[1] == 'a' && q[2] == 'g')
				q += 3;
			if (*q == '{') {
				strcpy(buf,"ValArray");
				doFile(buf,it);
			}
			else {
			        int i = 0;
				while (*q != ' ' && *q != '{' && *q)
					buf[i++] = *q++;
				buf[i]=0;
				q = strstr(buf,"Interface");
				if (q) *q=0;
				if (FindType(buf) >= 0) {
					doFile(buf,it);
				}
				else if (strstr(buf,"WDictionary")) {
					strcpy(buf,"WDictionary");
					doFile(buf,it);
				}
				else if (strstr(buf,"HeapAllocator")) {
					strcpy(buf,"ContainerHeap");
					doFile(buf,it);
				}
			}
		}
	}
         for (p = it->GetFirst(it); p != NULL; p = it->GetNext(it)) {
                 int r = ScanOneLine(p);
        }

	istrCollection.deleteIterator(it);
	istrCollection.Finalize(text);
}

static int destroyDataPoint(void *pdata)
{
	DataPoint *data = pdata;
	free(data->ApiName);
	return 1;
}

int main(void)
{
	Iterator *it;
	DataPoint *d;
	strCollection *Output = istrCollection.CreateWithAllocator(100,&iDebugMalloc);
	int j,APITotals=0;
	int totals[1+NbContainers];
	char buf[4096];

	memset(totals,0,sizeof(totals));
	Exceptions = istrCollection.InitializeWith(sizeof(ExceptionsTable)/sizeof(ExceptionsTable[0]),ExceptionsTable);
	ContainerTable = istrCollection.InitializeWith(NbContainers,ContainerNames);
	ExtractIds("../containers.h");
	ExtractIds("../valarraygen.h");
	ExtractIds("../stringlistgen.h");
	it = iDictionary.NewIterator(Data);
	iDictionary.SetDestructor(Data,destroyDataPoint);
	for (d = it->GetFirst(it); d != NULL; d = it->GetNext(it)) {
		int len = strlen(d->ApiName);
		if (len > 15)
		sprintf(buf,"%-20s\t&",d->ApiName);
		else if (len > 12)
		sprintf(buf,"%-20s\t&",d->ApiName);
		else
		sprintf(buf,"%-20s\t&",d->ApiName);
		for (j=0; j<NbContainers;j++) {
			if (d->type[j]) {
				sprintf(buf+strlen(buf)," \\X ");
				totals[j] += 1;
			}
			else sprintf(buf+strlen(buf)," ");
			if (j != NbContainers-1)
				strcat(buf,"&");
		}
		strcat(buf,"\\\\\n");
		istrCollection.Add(Output,buf);
	}
	istrCollection.Sort(Output);
	istrCollection.Add(Output,"\\hline\n");
	sprintf(buf,"%-20s\t&","Totals");
	for (j=0; j<NbContainers;j++) {
		sprintf(buf+strlen(buf),"%3d&",totals[j]);
		APITotals += totals[j];
	}
	buf[strlen(buf)-1]=0; /* Eliminate the last '&' */
	strcat(buf,"\\\\\n");
	istrCollection.Add(Output,buf);

	sprintf(buf,"%-20s\t& %d & ","Total APIs",APITotals);
	for (j=0; j<NbContainers-2;j++) {
		sprintf(buf+strlen(buf)," & ");
	}
	strcat(buf,"\\\\\n");
	istrCollection.Add(Output,buf);
	istrCollection.WriteToFile(Output,"table.tex");
	istrCollection.Finalize(Output);
	iDictionary.deleteIterator(it);
	iDictionary.Finalize(Data);
	Data = NULL;
	return 0;
}
