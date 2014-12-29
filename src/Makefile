CXX=g++
CXXFLAGS = -Wall -g -O2 -std=c++11

OBJECTS = \
    asm.o \
    asm_local.o \
    ssa.o \
    ssa_adhoc.o \
    ssa_construction.o \
    ssa_copy_propagation.o \
    ssa_dead_code_elimination.o \
    ssa_deconstruction.o \
    ssa_reader.o \
    ssa_register_allocation.o \
    ssa_subroutine_builder.o \
    ssa_writer.o \
    CPU.o \
    JackParser.o \
    JackTokenizer.o \
    JackVMWriter.o \
    ParserVM.o \
    VMOptimize.o \
    VMWriter.o \
    VMCommand.o

BIN = asm2hack vm2asm jack2vm vm2hack emulator emulator-gui jack2ssa ssa2asm

all: Makefile.depend $(BIN)

Makefile.depend: *.cpp *.cc *.h
	$(CXX) $(CXXFLAGS) -MM *.cpp *.cc > $@

asm2hack: asm2hack.o $(OBJECTS)
	$(CXX) -o $@ $^

vm2hack: vm2hack.o $(OBJECTS)
	$(CXX) -o $@ $^

vm2asm: vm2asm.o $(OBJECTS)
	$(CXX) -o $@ $^

jack2vm: jack2vm.o $(OBJECTS)
	$(CXX) -o $@ $^

emulator: emulator.o $(OBJECTS)
	$(CXX) -o $@ $^

emulator-gui: emulator-gui.o $(OBJECTS)
	$(CXX) -o $@ $^ `pkg-config --libs gtk+-3.0` `pkg-config --libs glib-2.0`

jack2ssa: jack2ssa.o $(OBJECTS)
	$(CXX) -o $@ $^

ssa2asm: ssa2asm.o $(OBJECTS)
	$(CXX) -o $@ $^

emulator-gui.o: emulator-gui.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS) $(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --cflags glib-2.0)

%.o: %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

include Makefile.depend

clean:
	rm -f $(BIN)
	rm -f $(patsubst %,%.o,$(BIN))
	rm -f $(OBJECTS)
	rm -f Makefile.depend
