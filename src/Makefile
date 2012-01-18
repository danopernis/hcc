CXXFLAGS = -Wall -g

ASM_OBJECTS = asm.o AsmParser.o StringTable.o AsmLogic.o
VM_OBJECTS = vm.o VMParser.o VMCommand.o VMWriter.o VMOptimize.o cpu.o StringTable.o StageConnect.o
JACK_OBJECTS = jack.o JackTokenizer.o StringTable.o JackParser.o JackParserCallback.o JackVMWriter.o
VM_ASM_OBJECTS = vm2hack.o VMParser.o VMWriter.o VMOptimize.o AsmLogic.o StringTable.o cpu.o StageConnect.o
EMULATOR_OBJECTS = emulator.o cpu.o

all: asm2hack vm2asm jack2vm vm2asm2hack emulator

Makefile.depend:
	g++ -MM *.cpp > Makefile.depend

asm2hack: $(ASM_OBJECTS)
	g++ -o asm2hack $(ASM_OBJECTS)

vm2asm: $(VM_OBJECTS)
	g++ -o vm2asm $(VM_OBJECTS)

jack2vm: $(JACK_OBJECTS)
	g++ -o jack2vm $(JACK_OBJECTS)

vm2asm2hack: $(VM_ASM_OBJECTS)
	g++ -o vm2asm2hack $(VM_ASM_OBJECTS)

emulator: $(EMULATOR_OBJECTS)
	g++ -o emulator $(EMULATOR_OBJECTS)

%.o: %.cpp
	g++ $(CXXFLAGS) -o $@ -c $<

include Makefile.depend

clean:
	rm -f asm2hack $(ASM_OBJECTS)
	rm -f vm2asm $(VM_OBJECTS)
	rm -f jack2vm $(JACK_OBJECTS)
	rm -f vm2asm2hack $(VM_ASM_OBJECTS)
	rm -f emulator $(EMULATOR_OBJECTS)
	rm -f Makefile.depend
