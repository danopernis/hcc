CXXFLAGS = -Wall -g

ASM_OBJECTS = asm.o AsmParser.o
VM_OBJECTS = vm.o VMParser.o VMCommand.o VMWriter.o VMOptimize.o cpu.o StringTable.o

all: hcc-asm hcc-vm

Makefile.depend:
	g++ -MM *.cpp > Makefile.depend

hcc-asm: $(ASM_OBJECTS)
	g++ -o hcc-asm $(ASM_OBJECTS)

hcc-vm: $(VM_OBJECTS)
	g++ -o hcc-vm $(VM_OBJECTS)

%.o: %.cpp
	g++ $(CXXFLAGS) -o $@ -c $<

include Makefile.depend

clean:
	rm -f hcc-asm $(ASM_OBJECTS)
	rm -f hcc-vm $(VM_OBJECTS)
	rm -f Makefile.depend
