CXXFLAGS = -Wall -g -O2 -std=c++11

ASM_OBJECTS = \
	asm.o \
	asm_local.o

JACK_OBJECTS = \
	jack_parser.o \
	jack_token_type.o \
	jack_tokenizer.o \
	JackVMWriter.o

SSA_OBJECTS = \
	graph_dominance.o \
	ssa.o \
	ssa_asm_writer.o \
	ssa_construction.o \
	ssa_copy_propagation.o \
	ssa_dead_code_elimination.o \
	ssa_deconstruction.o \
	ssa_jack_reader.o \
	ssa_reader.o \
	ssa_register_allocation.o \
	ssa_subroutine_builder.o \
	ssa_tokenizer.o \
	ssa_writer.o

VM_OBJECTS = \
	ParserVM.o \
	VMOptimize.o \
	VMWriter.o \
	VMCommand.o \
	CPU.o

BIN = asm2hack vm2asm jack2vm vm2hack emulator emulator-gui hcc

all: Makefile.depend $(BIN)

Makefile.depend: *.cc *.h
	$(CXX) $(CXXFLAGS) -MM *.cc > $@

asm.a: $(ASM_OBJECTS)
	$(AR) cr $@ $^
	ranlib $@

jack.a: $(JACK_OBJECTS)
	$(AR) cr $@ $^
	ranlib $@

ssa.a: $(SSA_OBJECTS)
	$(AR) cr $@ $^
	ranlib $@

vm.a: $(VM_OBJECTS)
	$(AR) cr $@ $^
	ranlib $@

asm2hack: asm2hack.cc asm.a
	$(CXX) $(CXXFLAGS) -o $@ $^

vm2hack: vm2hack.cc asm.a vm.a
	$(CXX) $(CXXFLAGS) -o $@ $^

vm2asm: vm2asm.cc asm.a vm.a
	$(CXX) $(CXXFLAGS) -o $@ $^

jack2vm: jack2vm.cc jack.a vm.a
	$(CXX) $(CXXFLAGS) -o $@ $^

emulator: emulator.cc CPU.o
	$(CXX) $(CXXFLAGS) -o $@ $^

emulator-gui: emulator-gui.cc CPU.o
	$(CXX) $(CXXFLAGS) -o $@ $^ `pkg-config --libs --cflags gtk+-3.0`

hcc: hcc.cc asm.a jack.a ssa.a
	$(CXX) $(CXXFLAGS) -o $@ $^

-include Makefile.depend

TESTS = \
	test_graph \
	test_interference_graph \
	test_jack_tokenizer \
	test_ssa_integration \
	test_ssa_tokenizer \
	test_vm_integration

check: $(TESTS)
	./test_graph
	./test_interference_graph
	./test_jack_tokenizer
	./test_ssa_integration
	./test_ssa_tokenizer
	./test_vm_integration

test_%: test_%.cc asm.a jack.a ssa.a vm.a
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(BIN)
	rm -f Makefile.depend
	rm -f asm.a $(ASM_OBJECTS)
	rm -f jack.a $(JACK_OBJECTS)
	rm -f ssa.a $(SSA_OBJECTS)
	rm -f vm.a $(VM_OBJECTS)
	rm -f $(TESTS)
