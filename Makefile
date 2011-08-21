CXXFLAGS=-g -O -W -Wall -MMD -I.

EXES=dwarfzip dwarfstat

all: $(EXES)

check: all
	./runtests.sh

dwarfzip: binary.o scanner.o dwarfzip.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

dwarfstat: binary.o scanner.o dwarfstat.o dwarfstr.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

clean:
	rm -f *.o $(EXES)

-include *.d
