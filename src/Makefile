CXXFLAGS := $(CXXFLAGS) -Wall -g -O2 -std=c++11

SRC := $(wildcard *.cc)
OBJ := $(SRC:.cc=.o)

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
	ssa_superblock_clone.o \
	ssa_tokenizer.o \
	ssa_writer.o

VM_OBJECTS = \
	ParserVM.o \
	VMOptimize.o \
	VMWriter.o \
	VMCommand.o \
	CPU.o

BIN := jack2vm emulator hcc

all: $(BIN)

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

jack2vm: jack2vm.cc jack.a vm.a
	$(CXX) $(CXXFLAGS) -o $@ $^

emulator: emulator.cc CPU.o
	$(CXX) $(CXXFLAGS) -o $@ $^ `pkg-config --libs --cflags gtkmm-3.0`

hcc: hcc.cc asm.a jack.a ssa.a vm.a
	$(CXX) $(CXXFLAGS) -o $@ $^

include $(OBJ:.o=.d)

%.d: %.cc
	./depend.sh $(CXX) $(CXXFLAGS) $*.cc > $@

TESTS := \
	test_graph \
	test_interference_graph \
	test_jack_tokenizer \
	test_ssa_integration \
	test_ssa_tokenizer \
	test_vm_integration

check: all $(TESTS)
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
	rm -f asm.a $(ASM_OBJECTS)
	rm -f jack.a $(JACK_OBJECTS)
	rm -f ssa.a $(SSA_OBJECTS)
	rm -f vm.a $(VM_OBJECTS)
	rm -f $(TESTS)

install: all
	test -d $(DESTDIR)/bin
	cp -t $(DESTDIR)/bin $(BIN)

.PHONY: all check clean install
