# Copyright (c) 2012-2018 Dano Pernis

add_executable(test_jack_tokenizer test_jack_tokenizer.cc)
target_link_libraries(test_jack_tokenizer PRIVATE jack)
add_test(jack_tokenizer test_jack_tokenizer)

add_executable(test_ssa_integration test_ssa_integration.cc)
target_link_libraries(test_ssa_integration PRIVATE asm jack ssa vm)
add_test(ssa_integration test_ssa_integration)

add_executable(test_ssa_tokenizer test_ssa_tokenizer.cc)
target_link_libraries(test_ssa_tokenizer PRIVATE ssa)
add_test(ssa_tokenizer test_ssa_tokenizer)

add_executable(test_vm_integration test_vm_integration.cc)
target_link_libraries(test_vm_integration PRIVATE asm vm)
add_test(vm_integration test_vm_integration)
