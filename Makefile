CXXFLAGS=-g -O -W -Wall

EXES=dwarfzip

all: dwarfzip

dwarfzip: binary.o dwarfzip.o
	$(CXX) -o $@ $^

clean:
	rm -f *.o $(EXES)
