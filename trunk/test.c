#include "containers.h"
#include <stdio.h>
static void ABORT(char *file,int line)
{
	fprintf(stderr,"*****\n\nABORT\nFile %s Line %d\n**********\n\n",file,line);
	abort();
}
#define Abort() ABORT(__FILE__,__LINE__)
static void PrintList(List *l)
{
    Iterator *it = iList.NewIterator(l);
    int *pi;

    for (pi = it->GetFirst(it); pi != NULL; pi = it->GetNext(it)) {
        printf("%d ",*pi);
    }
    printf("\n");
}

static int testRemoveRange(void)
{
    List *l1;
    int table[] = {1,2,3,4,5,6,7,8,9,10};

    l1 = iList.InitializeWith(sizeof(int),10,table);
    iList.Copy(l1);
    printf("Original list:\n");
    PrintList(l1);
    printf("Removing element 2 to 5\n");
    iList.RemoveRange(l1,2,5);
    PrintList(l1);
    return 1;
}
static void testList(void)
{
	FILE *outFile;
	List *l = iList.Create(sizeof(double)),*l1,*l2;
	double d,sum;
	const double *pd;
	size_t isum=0,msum=0,i;

        testRemoveRange();
	for (i=0; i<1000;i++) {
		d = i;
		isum += i;
		iList.Add(l,(void *)&d);
	}
	if (iList.Size(l) != 1000) {
		Abort();
	}
	sum = 0.0;
	for (i=0; i<1000;i++) {
		pd = iList.GetElement(l,i);
		sum += *pd;
	}
	if (sum != isum)
		Abort();
	sum=0;
	for (i=0; i< 500;i++) {
		iList.PopFront(l,&d);
		msum += i;
		sum += d;

	}
	if (msum != sum)
		Abort();
	if ( iList.Size(l) != 500) {
		Abort();
	}
	l1 = iList.GetRange(l,0,500);
	if (!iList.Equal(l,l1))
		Abort();
	l2 = iList.Copy(l);
	if (!iList.Equal(l2,l1))
		Abort();
	for (i=0; i<iList.Size(l); i++) {
		d = *(double *)iList.GetElement(l,i);
		if (d != i+500) {
			Abort();
		}
	}
	outFile = fopen("iListsave","wb");
	iList.Save(l2,outFile,NULL,NULL);
	fclose(outFile);
	iList.Reverse(l1);
	d = 600.0;
	iList.Erase(l,&d);
	if (iList.Size(l) != 499)
		Abort();
	iList.Finalize(l1);
	outFile = fopen("iListsave","rb");
	l1 = iList.Load(outFile,NULL,NULL);
	fclose(outFile);
	if (!iList.Equal(l2,l1))
		Abort();
	if (iList.Size(l1) != 500)
		Abort();
	i = iList.Size(l);
	while (i>0) {
		iList.PopFront(l,&d);
		i--;
	}
	if (iList.Size(l) != 0)
		Abort();
	iList.Finalize(l1);
	iList.Finalize(l2);
	iList.Finalize(l);
}


static void PrintVector(Vector *AL)
{
	size_t i;
	printf("Count %ld, Capacity %ld\n",(long)iVector.Size(AL),(long)iVector.GetCapacity(AL));
	for (i=0; i<iVector.Size(AL);i++) {
		printf("%s\n",*(char **)iVector.GetElement(AL,i));
	}
	printf("\n");
}

static int compareStrings(const void *s1,const void *s2,CompareInfo *ExtraArgs)
{
	char **str1=(char **)s1,**str2=(char **)s2;
	return strcmp(*str1,*str2);
}

char *Table[] = {
	"Martin",
	"Jakob",
	"Position 1",
	"Position 2",
	"pushed",
};
static int testVector(void)
{
	int errors=0;
	Vector *AL = iVector.Create(sizeof(void *),10);
	char **p;
	iVector.SetCompareFunction(AL,compareStrings);
	iVector.Add(AL,&Table[0]);
	iVector.Insert(AL,&Table[1]);
	if (!iVector.Contains(AL,&Table[0],NULL)) {
		Abort();
	}
	if (2 != iVector.Size(AL))
		Abort();
	iVector.InsertAt(AL,1,&Table[2]);
	iVector.InsertAt(AL,2,&Table[3]);
	if (4 != iVector.Size(AL))
		Abort();
	iVector.Erase(AL,&Table[1]);
	/*PrintVector(AL);*/
	iVector.PushBack(AL,&Table[4]);
	/*PrintVector(AL);*/
	iVector.PopBack(AL,NULL);
	/*PrintVector(AL);*/
	p = iVector.GetElement(AL,1);
	printf("Item position 1:%s\n",*p);
	PrintVector(AL);
	iVector.Finalize(AL);
	return errors;
}

#if 0
static void testVectorPerformance(void)
{
#define MAX_IT 50000000

	Vector *l = iVector.Create(sizeof(int),500000);
	size_t i;
	long long sum=0;
	for (i=0; i<MAX_IT; i++) {
		iVector.Add(l,&i);
	}
	for (i=0; i<iVector.Size(l); i++) {
		sum += *(int *)iVector.GetElement(l,i);
	}
	printf("sum is: %lld\n",sum);
/*getchar();*/
}

static int printStr(char *str,void *file)
{
	FILE *outfile = file;
	fprintf(outfile,"%s\n",str);
	return 1;
}
#endif
#if 0
static void printstrCollection(strCollection *sc)
{
	istrCollection.Apply(sc,printStr,stdout);
}


static int LoadSavestrCollection(strCollection *sc)
{
	strCollection *result;
	FILE *f = fopen("strCollection.txt","wb");
	if (f == NULL)
		return 1;
	if (istrCollection.Save(sc,f,NULL,NULL) < 0)
		return 1;
	fclose(f);
	f = fopen("strCollection.txt","rb");
	if (f == NULL)
		return 1;
	result = istrCollection.Load(f,NULL,NULL);
	if (result == NULL)
		return 1;
	if (!istrCollection.Equal(result,sc))
		return 1;
	fclose(f);
	remove("strCollection.txt");
	return 0;
}
#endif
static void PrintBitstring(BitString *b,char *doc)
{
	unsigned char buf[512];

	iBitString.Print(b,sizeof(buf),(unsigned char *)buf);
	printf("%-13.13s%65s\n",doc,buf);
}
static void TestBitstring(void){
	unsigned char *s = (unsigned char *)"1111 1010  0101 1100  0011 1001  0110 1111  1010 0101";
	BitString *b = iBitString.StringToBitString(s);
	BitString *original_b = iBitString.Copy(b);
	BitString *c = iBitString.Copy(b);
	BitString *d,*m;
	size_t i;
	unsigned char buf[512];

	m = iBitString.Create(10000);
	for (i=0; i<10000;i++) {
		iBitString.Add(m,1);
	}
	if (iBitString.GetElement(m,0) != 1) 
		printf("error bitstring\n");
	iBitString.Print(b,sizeof(buf),buf);
	if (strcmp((char *)s,(char *)buf)) {
		printf("Error in input/output\n");
	}
	if (!iBitString.Equal(b,c)) {
		printf("b and c are not equal?\n");
	}
	PrintBitstring(b,"Original b:");
	iBitString.EraseAt(b,4);
	PrintBitstring(b,"RemoveAt(b,4)");
	iBitString.InsertAt(b,4,1);
	PrintBitstring(b,"Insert(b,4,1)");
	printf("\n\n\n");
	PrintBitstring(c,"c:");
	iBitString.Add(c,1);
	PrintBitstring(c,"Add 1: c");
	iBitString.BitLeftShift(c,5);
	PrintBitstring(c,"LeftShift(c,5)");
	d = iBitString.GetRange(b,6,10);
	PrintBitstring(d,"GetRange(b,4):d");
	iBitString.IndexOf(b,1,NULL,&i);
	printf("Index of 1 in b is %d\n",(int)i);
	iBitString.IndexOf(b,0,NULL,&i);
	printf("Index of 0 in b is %d\n",(int)i);
	i = (size_t)iBitString.PopulationCount(b);
	printf("Population count of original b is %d\n",(int)i);
	i = (size_t)iBitString.BitBlockCount(original_b);
	printf("Bit block count of original b is %d\n",(int)i);
	iBitString.Finalize(b);
	iBitString.Finalize(c);
	iBitString.Finalize(d);
	b = iBitString.StringToBitString((unsigned char *)"011");
	c = iBitString.StringToBitString((unsigned char *)"1101101");
	d = iBitString.And(c,b);
	PrintBitstring(d,"011 AND 1101101");
	i = iBitString.BitBlockCount(c);
	printf("The block count of c is %ld\n",i);
	iBitString.Finalize(b);
	iBitString.Finalize(c);
	iBitString.Finalize(d);
	iBitString.Finalize(m);
	iBitString.Finalize(original_b);
}

#include <stdio.h>
static void PrintstrCollection(strCollection *SC){
    size_t i;
    printf("Count %d, Capacity %d\n",(int)istrCollection.Size(SC),(int)istrCollection.GetCapacity(SC));
    for (i=0; i<istrCollection.Size(SC);i++) {
        printf("%s\n",istrCollection.GetElement(SC,i));
    }
    printf("\n");
}
static void teststrCollection(void)
{
    strCollection *SC = istrCollection.Create(10);
    const char *p; char buf[40];
	size_t idx;
    istrCollection.Add(SC,"Martin");
    istrCollection.Insert(SC,(char *)"Jakob");
    if (!istrCollection.Contains(SC,(char *)"Martin")) {
        Abort();
    }
    if (2 != istrCollection.Size(SC))
		Abort();
    istrCollection.InsertAt(SC,1,(char *)"Position 1");
    istrCollection.InsertAt(SC,2,(char *)"Position 2");
	if (0 == istrCollection.Contains(SC,(char *)"Position 1"))
		Abort();
	istrCollection.IndexOf(SC,(char *)"Position 2",&idx);
	if (idx != 2)
		Abort();
	if (4 != istrCollection.Size(SC))
		Abort();
    istrCollection.Erase(SC,(char *)"Jakob");
    if (istrCollection.Contains(SC,(char *)"Jakob"))
		Abort();
	if (3 != istrCollection.Size(SC))
		Abort();
    istrCollection.PushFront(SC,(char *)"pushed");
	if (4 != istrCollection.Size(SC))
		Abort();
	istrCollection.IndexOf(SC,(char *)"pushed",&idx);
	if (0 != idx)
		Abort();
    istrCollection.PopFront(SC,(char *)buf,sizeof(buf));
	if (3 != istrCollection.Size(SC))
		Abort();
	if (strcmp((char *)buf,"pushed"))
		Abort();
    PrintstrCollection(SC);
    p = istrCollection.GetElement(SC,1);
    printf("Item position 1:%s\n",p);
    PrintstrCollection(SC);
	istrCollection.Finalize(SC);
#if 0
	/* Here you should add a file path (text file)
	   that can be used to test the string collection */
	SC = istrCollection.CreateFromFile((unsigned char *)"../../test.c");
	PrintstrCollection(SC);
	istrCollection.Finalize(SC);
#endif
}

static int TestDictionary(void)
{
	Dictionary *d = iDictionary.Create(sizeof(int *),30);
	int data[12];
	size_t count;
	int *pi,sum,r;
	Iterator *it;

	data[1] = 1;
	data[2] = 2;
	iDictionary.Add(d,"One",&data[1]);
	iDictionary.Add(d,"Two",&data[2]);
	pi = (int *)iDictionary.GetElement(d,"Two");
	if (*pi != 2)
		Abort();
	pi = (int *)iDictionary.GetElement(d,"One");
	if (*pi != 1)
		Abort();
	count = iDictionary.Size(d);
	if (count != 2)
		Abort();
	it = iDictionary.NewIterator(d);
	sum = 0;
	for (pi = it->GetFirst(it);
		pi != NULL; pi = it->GetNext(it)) {
		sum += *pi;
	}
	iDictionary.deleteIterator(it);
	if (sum != 3)
		Abort();
	r=iDictionary.Erase(d,"long data");
	if (r != CONTAINER_ERROR_NOTFOUND)
		Abort();
	count = iDictionary.Size(d);
	if (count != 2)
		Abort();
	iDictionary.Erase(d,"One");
	count = iDictionary.Size(d);
	if (count != 1)
		Abort();
	iDictionary.Finalize(d);
	return 0;
}


static int compareDoubles(const void *d1,const void *d2,CompareInfo *arg)
{
	double a = *(double *)d1;
	double b = *(double *)d2;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	return 0;
}

static int printDoubleTree(const void *data,void *arg)
{
	FILE *f = (FILE *)arg;
	fprintf(f,"%g ",*(double *)data);
	return 0;
}
#undef MAX_IT
#define MAX_IT 10
static void testBinarySearchTree(void)
{
	BinarySearchTree *tree = iBinarySearchTree.Create(sizeof(double));
	BinarySearchTree *tree1 = iBinarySearchTree.Create(sizeof(double));
	double d = 1.0;
	size_t i;

	iBinarySearchTree.SetCompareFunction(tree,compareDoubles);
	for (i=0; i<MAX_IT;i++) {
		d = i+1;
		iBinarySearchTree.Add(tree,&d);
	}
	if (iBinarySearchTree.Size(tree) != i)
		Abort();
	iBinarySearchTree.Apply(tree,printDoubleTree,stdout);
	printf("Size: %lu\n",iBinarySearchTree.Sizeof(tree));
	iBinarySearchTree.SetCompareFunction(tree1,compareDoubles);
	for (i=0; i<MAX_IT;i++) {
		d = i+1;
		iBinarySearchTree.Add(tree1,&d);
	}
	if (!iBinarySearchTree.Equal(tree1,tree))
		printf("Error in comparison of binary search trees\n");
	d = 2.0;
	iBinarySearchTree.Erase(tree,&d,NULL);
	iBinarySearchTree.Apply(tree,printDoubleTree,stdout);
	iBinarySearchTree.Finalize(tree);
	iBinarySearchTree.Finalize(tree1);
}

static void testScapegoatTree(void)
{
	TreeMap *tree = iTreeMap.Create(sizeof(double));
	TreeMap *tree1 = iTreeMap.Create(sizeof(double));
	double d = 1.0;
	size_t i;

	iTreeMap.SetCompareFunction(tree,compareDoubles);
	for (i=0; i<MAX_IT;i++) {
		d = i+1;
		iTreeMap.Add(tree,&d,NULL);
	}
	if (iTreeMap.Size(tree) != i)
		Abort();
	iTreeMap.Apply(tree,printDoubleTree,stdout);
	printf("Size: %lu\n",iTreeMap.Sizeof(tree));
	iTreeMap.SetCompareFunction(tree1,compareDoubles);
	for (i=0; i<MAX_IT;i++) {
		d = i+1;
		iTreeMap.Add(tree1,&d,NULL);
	}
#if 0
	if (!iTreeMap.Equal(tree1,tree))
		printf("Error in comparison of binary search trees\n");
#endif
	d = 2.0;
	iTreeMap.Erase(tree,&d,NULL);
	iTreeMap.Apply(tree,printDoubleTree,stdout);
	iTreeMap.Finalize(tree);
	iTreeMap.Finalize(tree1);
}

static int testBloomFilter(void)
{
	BloomFilter *b = iBloomFilter.Create(10,0.00001);
	int i,errors=0;
	i = 4734;
	iBloomFilter.Add(b,&i,sizeof(int));
	i = 9457;
	iBloomFilter.Add(b,&i,sizeof(int));
	i = 458223;
	iBloomFilter.Add(b,&i,sizeof(int));
	i = 40774;
	iBloomFilter.Add(b,&i,sizeof(int));
	i = 9334422;
	iBloomFilter.Add(b,&i,sizeof(int));
	i = 9334422;
	if (!iBloomFilter.Find(b,&i,sizeof(int))) {
		Abort();
	}
	i = 4734;
	if (!iBloomFilter.Find(b,&i,sizeof(int))) {
		Abort();
	}
	i = 9;
	if (iBloomFilter.Find(b,&i,sizeof(int))) {
		Abort();
	}
	iBloomFilter.Finalize(b);
	return errors;
}

static int testStreamBuffers(void)
{
	StreamBuffer *sb = iStreamBuffer.Create(10);
	int i;
	char buf[20];const char *p;
	
	for (i=0; i<10; i++) {
		sprintf(buf,"item %d",i+1);
		iStreamBuffer.Write(sb,buf,1+strlen(buf));
	}
	buf[0]=0;
	iStreamBuffer.Write(sb,&buf,1);
	printf("Buffer size is: %ld\n",iStreamBuffer.Size(sb));
	iStreamBuffer.SetPosition(sb,0);
	p = iStreamBuffer.GetData(sb);
	while (*p) {
		printf("%s\n",p);
		p += 1 + strlen(p);
	}
	iStreamBuffer.Finalize(sb);
	return 1;
}
int main(void)
{
#if 1
	int errors=0;
	CurrentAllocator = &iDebugMalloc;
	errors += testBloomFilter();
    teststrCollection();
	errors += testVector();
	testList();
	testBinarySearchTree();
	teststrCollection();
	TestDictionary();
	TestBitstring();
	testScapegoatTree();
	testStreamBuffers();
	/*RedBlackTree * rb = newRedBlackTree(20,5);*/
	/*rb->VTable->Finalize(rb);*/
	return errors;
#else

#endif
}
