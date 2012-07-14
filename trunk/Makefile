#----------------------------------------------------
#                  UNIX makefile
# Creates the library libccl.a and a small test program "dotest"
# 
#---------------------------------------------------
# Optimized CFLAGS setting
CFLAGS=-O2 -Wno-pointer-sign -DUNIX -Wall -D__MAC_OSX
CC=gcc
# Debug CFLAGS setting
#CFLAGS=-Wno-pointer-sign -DUNIX -Wall -g
SRC=	vector.c bloom.c error.c dlist.c qsortex.c heap.c \
	deque.c hashtable.c malloc_debug.c containers.h ccl_internal.h \
	stdint.h.noc99 pool.c pooldebug.c redblacktree.c scapegoat.c smallpool.c ccl_internal.h \
	bitstrings.c dictionarygen.c list.c memorymanager.c strcollection.c searchtree.c \
	containers.h ccl_internal.h redblacktree.c fgetline.c generic.c queue.c buffer.c observer.c \
	valarraydouble.c vectorsize_t.c valarrayint.c valarraylongdouble.c valarraygen.c \
	valarrayshort.c valarrayfloat.c valarrayuint.c valarraylonglong.c \
	valarrayulonglong.c sequential.c iMask.c wstrcollection.c strcollectiongen.c \
	stringlistgen.c stringlistgen.h stringlist.c stringlist.h wstringlist.h \
        priorityqueue.c intlist.c listgen.c 
DOCS=
MAKEFILES=Makefile Makefile.lcc Makefile.msvc

OBJS=vector.o error.o dlist.o qsortex.o bitstrings.o generic.o \
    dictionary.o wdictionary.o list.o strcollection.o searchtree.o heap.o malloc_debug.o \
    bloom.o fgetline.o pool.o pooldebug.o redblacktree.o scapegoat.o queue.o \
    buffer.o observer.o valarraydouble.o valarrayint.o vectorsize_t.o \
    valarraylongdouble.o valarrayshort.o valarrayfloat.o valarrayuint.o \
    valarraylonglong.o valarrayulonglong.o memorymanager.o sequential.o \
    iMask.o deque.o hashtable.o wstrcollection.o stringlist.o wstringlist.o \
    priorityqueue.o intlist.o doublelist.o longlonglist.o intdlist.o \
    doubledlist.o longlongdlist.o
LIST_GENERIC=listgen.c listgen.h
DLIST_GENERIC=dlistgen.c dlistgen.h
#DRAWINGS
PNGFILES=AuxiliaryInterfaces.png DListVocabulary.png Pool.png  bitstrings.png Basic.png \
        Dictionary.png  StreamBuffer.png list.png BloomFilter.png  Iterator.png  ValArray.png \
        Circular.png  ListVocabulary.png VectorVocabulary.png Containers.png  Memorymanagement.png\
        Vocabulary.png
#Documentation source files. Some of those are automatically generated
TEXFILES=BitString.tex Dlist.tex PQueue.tex ValArray.tex \
ContainerHeap.tex HashTable.tex Queue.tex Vector.tex rgb.tex\
Deque.tex Introduction.tex StreamBuffer.tex WDictionary.tex strcollection.tex\
Dictionary.tex List.tex TreeMap.tex ccl.tex  table.tex

#----------------------------------IMPORTANT --------------------
####
#### Please remove ccl.pdf from the targets if you do not have the TeX system installed
####
#-----------------------------------IMPORTANT --------------------
all: 	libccl.a dotest ccl.pdf
dotest:	libccl.a test.o 
	gcc -o dotest -g $(CFLAGS) test.c libccl.a -lm
libccl.a:	$(OBJS) containers.h ccl_internal.h ccl_internal.h
	ar r libccl.a $(OBJS)
clean:
	rm -rf $(OBJS) libccl.a dotest dotest.dSYM ccl.zip ccl.pdf ccl.log ccl.aux ccl.toc ccl.ilg
zip:	$(SRC)
	rm ccl.zip;rm -rf ccl;svn export . ccl;/bin/sh dolinks.sh;zip -9 -r ccl.zip ccl 

valarraylongdouble.o:   valarraygen.c valarraylongdouble.c containers.h ccl_internal.h valarraygen.h valarray.h
valarraydouble.o:       valarraygen.c valarraydouble.c containers.h ccl_internal.h valarraygen.h valarray.h
valarrayint.o:          valarraygen.c valarrayint.c containers.h ccl_internal.h valarraygen.h valarray.h
valarrayshort.o:	valarraygen.c valarrayshort.c containers.h ccl_internal.h valarraygen.h valarray.h
vectorsize_t.o:       vectorgen.c vectorsize_t.c containers.h ccl_internal.h vectorgen.h 
valarrayfloat.o:	valarraygen.c valarrayfloat.c containers.h ccl_internal.h valarraygen.h valarray.h
valarrayuint.o:		valarraygen.c valarrayuint.c containers.h ccl_internal.h valarraygen.h valarray.h
valarraylonglong.o:	valarraygen.c valarraylonglong.c containers.h ccl_internal.h valarraygen.h valarray.h
valarrayulonglong.o:     valarraygen.c valarrayulonglong.c containers.h ccl_internal.h valarraygen.h valarray.h
observer.o:	containers.h ccl_internal.h observer.c 
buffer.o:	containers.h ccl_internal.h buffer.c
vector.o:	containers.h ccl_internal.h vector.c
buffer.o:	containers.h ccl_internal.h buffer.c
vector.o:	containers.h ccl_internal.h vector.c
bloom.o:	containers.h ccl_internal.h bloom.c
error.o:	containers.h ccl_internal.h error.c
dlist.o:		dlist.c containers.h ccl_internal.h
deque.o:	deque.c containers.h ccl_internal.h
hashtable.o:	hashtable.c	containers.h ccl_internal.h
dlist.o:	dlist.c containers.h ccl_internal.h
list.o:		list.c containers.h ccl_internal.h
dictionary.o:	dictionary.c dictionarygen.c containers.h ccl_internal.h
wdictionary.o:	wdictionary.c dictionarygen.c containers.h ccl_internal.h
qsortex.o:	qsortex.c containers.h ccl_internal.h
generic.o:	generic.c containers.h ccl_internal.h
heap.o:	heap.c containers.h ccl_internal.h
memorymanager.o:	memorymanager.c containers.h ccl_internal.h
sequential.o:	sequential.c containers.h ccl_internal.h
iMask.o:	iMask.c containers.h ccl_internal.h
scapegoat.o:	scapegoat.c containers.h ccl_internal.h
wstrcollection.o:	wstrcollection.c strcollectiongen.c containers.h ccl_internal.h
strcollection.o:       strcollection.c strcollectiongen.c containers.h ccl_internal.h
stringlist.o:	stringlist.c stringlistgen.c containers.h ccl_internal.h stringlist.h stringlistgen.h
wstringlist.o:	wstringlist.c stringlistgen.c stringlistgen.h wstringlist.h containers.h ccl_internal.h
priorityqueue.o: priorityqueue.c ccl_internal.h containers.h
intlist.o:	intlist.h intlist.c ccl_internal.h containers.h $(LIST_GENERIC)
doublelist.o:	doublelist.h doublelist.c ccl_internal.h containers.h $(LIST_GENERIC)
longlonglist.o:	longlonglist.h longlonglist.c ccl_internal.h containers.h $(LIST_GENERIC)
intdlist.o:      intdlist.h intdlist.c ccl_internal.h containers.h $(LIST_GENERIC)
doubledlist.o:   doubledlist.h doubledlist.c ccl_internal.h containers.h $(DLIST_GENERIC)
longlongdlist.o: longlongdlist.h longlongdlist.c ccl_internal.h containers.h $(DLIST_GENERIC)

# This supposes that pdflatex and makeindex are in the path
# please change accordingly
ccl.pdf:	$(TEXFILES) $(PNGFILES) 
	makeindex ccl.idx;pdflatex ccl.tex
