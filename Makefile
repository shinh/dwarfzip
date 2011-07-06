all: dwarfzip

dwarfzip: binary.o dwarfzip.o
	$(CXX) -o $@ $^
