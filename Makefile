#----------------------------------------------------
#                  UNIX makefile
# Creates the library libccl.a and a small test program "dotest"
# 
#---------------------------------------------------
CFLAGS=-g -Wno-pointer-sign -DUNIX -Wall -pedantic 
SRC=	arraylist.c bloom.c containererror.c dlist.c qsortex.c test.c heap.c \
	deque.c hashtable.c malloc_debug.c containers.h \
	stdint.h pool.c pooldebug.c redblacktree.c scapegoat.c smallpool.c ccl_internal.h \
	bitstrings.c dictionary.c list.c strcollection.c searchtree.c \
	containers.h redblacktree.c fgetline.c generic.c queue.c
DOCS=	ccl.pdf 
MAKEFILES=Makefile Makefile.lcc Makefile.msvc

OBJS=arraylist.o containererror.o dlist.o qsortex.o test.o bitstrings.o generic.o \
    dictionary.o list.o strcollection.o searchtree.o heap.o malloc_debug.o \
    bloom.o fgetline.o pool.o pooldebug.o redblacktree.o scapegoat.o queue.o

dotest:	libccl.a test.c
	gcc -o dotest test.c libccl.a -lm
libccl.a:	$(OBJS) 
	ar r libccl.a $(OBJS)
clean:
	rm -f $(OBJS) libccl.a dotest
zip:	$(SRC)
	zip -9 container-lib-src.zip $(SRC) $(DOCS) $(MAKEFILES) CHANGELOG

