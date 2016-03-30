PNGS=AuxiliaryInterfaces.png DListVocabulary.png Pool.png  bitstrings.png Basic.png  Dictionary.png  StreamBuffer.png list.png BloomFilter.png  Iterator.png  ValArray.png Circular.png  ListVocabulary.png VectorVocabulary.png Containers.png  Memorymanagement.png Vocabulary.png

all:	ccl.pdf ccl.html

ccl.pdf:	ccl.tex table.tex $(PNGS) ../containers.h 
	/usr/texbin/pdflatex ccl.tex

table.tex:	dotable ../containers.h ../valarraygen.h 
	./dotable

ccl.html:	tth ccl.tex $(PNGS) table.tex
	./tth ccl.tex

dotable:	dotable.c
	gcc -g -o dotable -I.. dotable.c ../libccl.a

tth:	tth.c
	gcc -o tth -g tth.c
clean:
	rm -f table.tex ccl.pdf ccl.html dotable tth ccldoc.zip
zip:
	zip -9 ccldoc.zip dotable.c ccl.tex tth.c ../libccl.a $(PNGS) Makefile
