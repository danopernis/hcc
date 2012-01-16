CXXFLAGS = -Wall -g

ASM_OBJECTS = asm.o AsmParser.o StringTable.o AsmLogic.o
VM_OBJECTS = vm.o VMParser.o VMCommand.o VMWriter.o VMOptimize.o cpu.o StringTable.o StageConnect.o
EMULATOR_OBJECTS = emulator.o cpu.o
VM2HACK_OBJECTS = vm2hack.o VMParser.o VMWriter.o VMOptimize.o AsmLogic.o StringTable.o cpu.o StageConnect.o

all: hcc-asm hcc-vm hcc-emulator hcc-vm2hack

Makefile.depend:
	g++ -MM *.cpp > Makefile.depend

hcc-asm: $(ASM_OBJECTS)
	g++ -o hcc-asm $(ASM_OBJECTS)

hcc-vm: $(VM_OBJECTS)
	g++ -o hcc-vm $(VM_OBJECTS)

hcc-emulator: $(EMULATOR_OBJECTS)
	g++ -o hcc-emulator $(EMULATOR_OBJECTS)

hcc-vm2hack: $(VM2HACK_OBJECTS)
	g++ -o hcc-vm2hack $(VM2HACK_OBJECTS)

%.o: %.cpp
	g++ $(CXXFLAGS) -o $@ -c $<

include Makefile.depend

clean:
	rm -f hcc-asm $(ASM_OBJECTS)
	rm -f hcc-vm $(VM_OBJECTS)
	rm -f hcc-emulator $(EMULATOR_OBJECTS)
	rm -f hcc-vm2hack $(VM2HACK_OBJECTS)
	rm -f Makefile.depend
