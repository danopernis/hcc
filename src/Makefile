CXXFLAGS = -Wall -g

ASM_OBJECTS = asm.o AsmParser.o

all: hcc-asm

Makefile.depend:
	g++ -MM *.cpp > Makefile.depend

hcc-asm: $(ASM_OBJECTS)
	g++ -o hcc-asm $(ASM_OBJECTS)

%.o: %.cpp
	g++ $(CXXFLAGS) -o $@ -c $<

include Makefile.depend

clean:
	rm -f hcc-asm $(ASM_OBJECTS) Makefile.depend
