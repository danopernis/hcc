# libraries

add_library (asm
	asm.cc
	asm_local.cc
    )
target_include_directories (asm PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

add_library (jack
	JackVMWriter.cc
	jack_parser.cc
	jack_token_type.cc
	jack_tokenizer.cc
    )
target_include_directories (jack PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

add_library (ssa
	graph_dominance.cc
	ssa.cc
	ssa_asm_writer.cc
	ssa_construction.cc
	ssa_copy_propagation.cc
	ssa_dead_code_elimination.cc
	ssa_deconstruction.cc
	ssa_jack_reader.cc
	ssa_reader.cc
	ssa_register_allocation.cc
	ssa_subroutine_builder.cc
	ssa_tokenizer.cc
	ssa_writer.cc
    )
target_include_directories (ssa PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

add_library (vm
	CPU.cc
	ParserVM.cc
	VMCommand.cc
	VMOptimize.cc
	VMWriter.cc
    )
target_include_directories (vm PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

# executables

add_executable (jack2vm jack2vm.cc)
target_link_libraries (jack2vm PRIVATE jack vm)

add_executable (emulator
    emulator.cc
    CPU.cc
    )
target_include_directories (emulator PRIVATE ${GTKMM_INCLUDE_DIRS})
target_compile_options (emulator PRIVATE
    ${GTKMM_CFLAGS_OTHER}
    -Wno-deprecated-declarations
    -Wno-deprecated-register
    -Wno-overloaded-virtual
    )
target_link_libraries (emulator PRIVATE ${GTKMM_LDFLAGS} ${CMAKE_THREAD_LIBS_INIT})

add_executable (hcc hcc.cc)
target_link_libraries (hcc PRIVATE asm jack ssa vm)

install (TARGETS
    emulator
    hcc
    jack2vm
    DESTINATION bin)

# tests

add_executable (test_interference_graph test_interference_graph.cc)
add_test (interference_graph test_interference_graph)

add_executable (test_graph test_graph.cc)
add_test (graph test_graph)
