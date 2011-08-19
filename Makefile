CXXFLAGS=-g -O -W -Wall -MMD -I.

EXES=dwarfzip dwarfstat

all: $(EXES)

check: all
	./runtests.sh

dwarfzip: binary.o scanner.o dwarfzip.o
	$(CXX) -o $@ $^

dwarfstat: binary.o scanner.o dwarfstat.o dwarfstr.o
	$(CXX) -o $@ $^

clean:
	rm -f *.o $(EXES)

-include *.d
