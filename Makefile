#----------------------------------------------------
#                  UNIX makefile
# Creates the library libccl.a and a small test program "dotest"
# 
#---------------------------------------------------
# Optimized CFLAGS setting
CFLAGS=-O2 -Wno-pointer-sign -DUNIX -Wall
# Debug CFLAGS setting
#CFLAGS=-g -Wno-pointer-sign -DUNIX -Wall 
SRC=	vector.c bloom.c containererror.c dlist.c qsortex.c heap.c \
	deque.c hashtable.c malloc_debug.c containers.h \
	stdint.h pool.c pooldebug.c redblacktree.c scapegoat.c smallpool.c ccl_internal.h \
	bitstrings.c dictionary.c list.c strcollection.c searchtree.c \
	containers.h redblacktree.c fgetline.c generic.c queue.c buffer.c observer.c \
	valarraydouble.c valarraysize_t.c valarrayint.c valarraylongdouble.c valarraygen.c \
	valarrayshort.c valarrayfloat.c
DOCS=
MAKEFILES=Makefile Makefile.lcc Makefile.msvc

OBJS=vector.o containererror.o dlist.o qsortex.o bitstrings.o generic.o \
    dictionary.o list.o strcollection.o searchtree.o heap.o malloc_debug.o \
    bloom.o fgetline.o pool.o pooldebug.o redblacktree.o scapegoat.o queue.o \
    buffer.o observer.o valarraydouble.o valarrayint.o valarraysize_t.o \
    valarraylongdouble.o valarrayshort.o valarrayfloat.o

dotest:	libccl.a test.o
	gcc -o dotest $(CFLAGS) test.c libccl.a -lm
libccl.a:	$(OBJS) containers.h ccl_internal.h
	ar r libccl.a $(OBJS)
clean:
	rm -rf $(OBJS) libccl.a dotest dotest.dSYM
zip:	$(SRC)
	rm container-lib-src.zip;rm -rf ccl;svn export . ccl;zip -9 -r  container-lib-src.zip ccl 

valarraylongdouble.o:   valarraygen.c valarraylongdouble.c containers.h valarraygen.h
valarraydouble.o:       valarraygen.c valarraydouble.c containers.h valarraygen.h
valarrayint.o:          valarraygen.c valarrayint.c containers.h valarraygen.h
valarrayshort.o:	valarraygen.c valarrayshort.c containers.h valarraygen.h
valarraysize_t.o:       valarraygen.c valarraysize_t.c containers.h valarraygen.h
valarrayfloat.o:	valarraygen.c valarrayfloat.c containers.h valarraygen.h
observer.o:	containers.h observer.c valarraygen.h
buffer.o:	containers.h buffer.c

