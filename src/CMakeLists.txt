# Copyright (c) 2012-2018 Dano Pernis

# libraries

add_library (assembler
    hcc/assembler/asm.cc
    hcc/assembler/asm.local.cc
    )
target_include_directories (assembler PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

add_library (cpu
    hcc/cpu/cpu.cc
    hcc/cpu/instruction.cc
    )
target_include_directories (cpu PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

add_library (jack
    hcc/jack/ast.cc
    hcc/jack/parser.cc
    hcc/jack/tokenizer.cc
    hcc/jack/tokenizer.token_type.cc
    hcc/jack/vm_writer.cc
    )
target_include_directories (jack PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

add_library (ssa
    hcc/ssa/interference_graph.cc
    hcc/ssa/ssa.asm_writer.cc
    hcc/ssa/ssa.cc
    hcc/ssa/ssa.construction.cc
    hcc/ssa/ssa.copy_propagation.cc
    hcc/ssa/ssa.dead_code_elimination.cc
    hcc/ssa/ssa.deconstruction.cc
    hcc/ssa/ssa.jack_reader.cc
    hcc/ssa/ssa.reader.cc
    hcc/ssa/ssa.register_allocation.cc
    hcc/ssa/ssa.writer.cc
    hcc/ssa/subroutine_builder.cc
    hcc/ssa/tokenizer.cc
    )
target_include_directories (ssa PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries (ssa PRIVATE util)

add_library (util
    hcc/util/graph_dominance.cc
    )
target_include_directories (util PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

add_library (vm
    hcc/vm/parser.cc
    hcc/vm/command.cc
    hcc/vm/optimize.cc
    hcc/vm/writer.cc
    )
target_include_directories (vm PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries (vm PRIVATE cpu)

# executables

add_executable (jack2vm jack2vm.cc)
target_link_libraries (jack2vm PRIVATE jack vm)

add_executable (emulator
    emulator.cc
    )
target_include_directories (emulator PRIVATE ${GTKMM_INCLUDE_DIRS})
target_compile_options (emulator PRIVATE
    ${GTKMM_CFLAGS_OTHER}
    -Wno-deprecated-declarations
    -Wno-deprecated-register
    -Wno-overloaded-virtual
    )
target_link_libraries (emulator PRIVATE ${GTKMM_LDFLAGS} ${CMAKE_THREAD_LIBS_INIT} cpu)

add_executable (hcc hcc.cc)
target_link_libraries (hcc PRIVATE assembler jack ssa vm)

install (TARGETS
    emulator
    hcc
    jack2vm
    DESTINATION bin)

# tests

add_executable (interference_graph.test hcc/ssa/interference_graph.test.cc)
target_link_libraries (interference_graph.test PRIVATE ssa)
add_test (interference_graph interference_graph.test)

add_executable (graph.test hcc/util/graph.test.cc)
target_link_libraries (graph.test PRIVATE util)
add_test (graph graph.test)
