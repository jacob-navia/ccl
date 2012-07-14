MAKEINDEX=/usr/texbin/makeindex
PDFLATEX=/usr/texbin/pdflatex
PNGS=AuxiliaryInterfaces.png DListVocabulary.png Pool.png  bitstrings.png Basic.png  Dictionary.png  StreamBuffer.png list.png BloomFilter.png  Iterator.png  ValArray.png Circular.png  ListVocabulary.png VectorVocabulary.png Containers.png  Memorymanagement.png Vocabulary.png

all:	ccl.pdf ccl.html

ccl.pdf:	ccl.tex table.tex $(PNGS) ../containers.h ccl.idx
	$(PDFLATEX) ccl.tex;$(MAKEINDEX) ccl.idx;

table.tex:	dotable ../containers.h ../valarraygen.h 
	./dotable

ccl.html:	tth ccl.tex $(PNGS) table.tex
	./tth ccl.tex

dotable:	dotable.c
	gcc -g -o dotable -I.. dotable.c ../libccl.a

tth:	tth.c
	gcc -o tth -g tth.c
clean:
	rm -f table.tex ccl.pdf ccl.html dotable tth ccldoc.zip ccl.tid ccl.idx ccl.log ccl.toc ccl.tin ccl.aux ccl.tms ccl.ilg
zip:
	zip -9 ccldoc.zip dotable.c ccl.tex tth.c ../libccl.a $(PNGS) Introduction.tex Makefile
