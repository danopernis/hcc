CXX=g++-4.7.2
CXXFLAGS = -Wall -g -std=c++11 $(shell pkg-config --cflags gtk+-3.0) $(shell pkg-config --cflags glib-2.0)

ASM2HACK_OBJECTS = asm2hack.o ParserAsm.o Assembler.o
VM2HACK_OBJECTS = vm2hack.o ParserVM.o VMWriter.o VMOptimize.o Assembler.o CPU.o StageConnect.o instruction.o
VM2ASM_OBJECTS = vm2asm.o ParserVM.o VMCommand.o VMWriter.o VMOptimize.o CPU.o StageConnect.o instruction.o
JACK2VM_OBJECTS = jack2vm.o JackTokenizer.o JackParser.o JackVMWriter.o
EMULATOR_OBJECTS = emulator.o CPU.o
EMULATOR_GUI_OBJECTS = emulator-gui.o CPU.o

all: asm2hack vm2asm jack2vm vm2hack emulator emulator-gui

Makefile.depend:
	$(CXX) -MM *.cpp > Makefile.depend

asm2hack: $(ASM2HACK_OBJECTS)
	$(CXX) -o asm2hack $^

vm2hack: $(VM2HACK_OBJECTS)
	$(CXX) -o vm2hack $^

vm2asm: $(VM2ASM_OBJECTS)
	$(CXX) -o vm2asm $^

jack2vm: $(JACK2VM_OBJECTS)
	$(CXX) -o jack2vm $^

emulator: $(EMULATOR_OBJECTS)
	$(CXX) -o emulator $^

emulator-gui: $(EMULATOR_GUI_OBJECTS)
	$(CXX) -o emulator-gui $^ `pkg-config --libs gtk+-3.0` `pkg-config --libs glib-2.0`

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

include Makefile.depend

clean:
	rm -f asm2hack $(ASM2HACK_OBJECTS)
	rm -f vm2hack $(VM2HACK_OBJECTS)
	rm -f vm2asm $(VM2ASM_OBJECTS)
	rm -f jack2vm $(JACK2VM_OBJECTS)
	rm -f emulator $(EMULATOR_OBJECTS)
	rm -f emulator-gui $(EMULATOR_GUI_OBJECTS)
	rm -f Makefile.depend
