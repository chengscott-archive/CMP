CC = g++ -std=c++11 -Ofast -Wall
CC0 = g++ -std=c++11 -g -Wall
OBJ = main.o memory.o irfile.o
archiTA = ../archiTA
goldensim = $(archiTA)/simulator/CMP

all: CMP

CMP: ${OBJ}
	$(CC) -o CMP $^

%.o: %.cpp
	$(CC) -c $^

.PNOHY: run
run: clean golden CMP
	./CMP
	diff snapshot.rpt diff/snapshot.rpt
	diff report.rpt diff/report.rpt

.PNOHY: golden
golden: dimage.bin iimage.bin
	./$(goldensim)
	mv *.rpt diff/

.PNOHY: test
test: clean CMP
	make -f makefile.test

.PHONY: clean
clean:
	rm -f CMP *.o *.rpt
