CXX=g++
CXXFLAGS = -Wall -g -O2 -std=c++11

OBJECTS = \
    instruction.o \
    Assembler.o \
    CPU.o \
    JackParser.o \
    JackTokenizer.o \
    JackVMWriter.o \
    ParserAsm.o \
    ParserVM.o \
    StageConnect.o \
    VMOptimize.o \
    VMWriter.o \
    VMCommand.o

BIN = asm2hack vm2asm jack2vm vm2hack emulator emulator-gui

all: $(BIN)

Makefile.depend:
	$(CXX) -MM *.cpp > $@

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
