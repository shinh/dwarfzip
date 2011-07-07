CXXFLAGS=-g -O -W -Wall

EXES=dwarfzip dwarfstat

all: $(EXES)

dwarfzip: binary.o scanner.o dwarfzip.o
	$(CXX) -o $@ $^

dwarfstat: binary.o scanner.o dwarfstat.o dwarfstr.o
	$(CXX) -o $@ $^

clean:
	rm -f *.o $(EXES)
