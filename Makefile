all: dwarfzip

dwarfzip: binary.o
	$(CXX) -o $@ $^
