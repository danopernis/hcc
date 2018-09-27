// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#include "hcc/ssa/ssa.h"

#include <ostream>
#include <sstream>

namespace hcc {
namespace ssa {

const std::map<instruction_type, std::string> type_to_string = {
    {instruction_type::ARGUMENT, "argument"},
    {instruction_type::JUMP, "jump"},
    {instruction_type::JLT, "jlt"},
    {instruction_type::JEQ, "jeq"},
    {instruction_type::CALL, "call"},
    {instruction_type::RETURN, "return"},
    {instruction_type::LOAD, "load"},
    {instruction_type::STORE, "store"},
    {instruction_type::MOV, "mov"},
    {instruction_type::ADD, "add"},
    {instruction_type::SUB, "sub"},
    {instruction_type::AND, "and"},
    {instruction_type::OR, "or"},
    {instruction_type::NEG, "neg"},
    {instruction_type::NOT, "not"},
    {instruction_type::PHI, "phi"},
};

void unit::save(std::ostream& output)
{
    auto write_bb = [&](const basic_block& bb, subroutine_ir&) {
        output << "block " << argument(bb.name).save_fast() << "\n";
        for (const auto& instr : bb.instructions) {
            output << "\t" << instr.save_fast() << " ;\n";
        }
    };

    for (auto& subroutine : subroutines) {
        output << "define " << argument(subroutine.first).save_fast() << "\n";
        auto& entry_block = subroutine.second.entry_node();
        auto& exit_block = subroutine.second.exit_node();
        write_bb(entry_block, subroutine.second);
        subroutine.second.for_each_bb([&](const basic_block& bb) {
            if (bb.name == entry_block.name || bb.name == exit_block.name
                || bb.instructions.empty()) {
                return;
            }

            write_bb(bb, subroutine.second);
        });
    }
}

std::string instruction::save_fast() const
{
    std::stringstream os;
    os << type_to_string.at(type);
    for (auto&& arg : arguments) {
        os << ' ' << arg.save_fast();
    }
    return os.str();
}

std::string save_fast_internal(char c, int index) { return c + std::to_string(index); }

std::string constant::save_fast() const { return save_fast_internal('#', value); }
std::string reg::save_fast() const { return save_fast_internal('%', index); }
std::string global::save_fast() const { return save_fast_internal('@', index); }
std::string local::save_fast() const { return save_fast_internal('&', index); }
std::string label::save_fast() const { return save_fast_internal('$', index); }

std::string argument::save_fast() const
{
    switch (type) {
    case argument_type::CONSTANT:
        return value.constant_value.save_fast();
    case argument_type::REG:
        return value.reg_value.save_fast();
    case argument_type::GLOBAL:
        return value.global_value.save_fast();
    case argument_type::LOCAL:
        return value.local_value.save_fast();
    case argument_type::LABEL:
        return value.label_value.save_fast();
    }
}

} // namespace ssa {
} // namespace hcc {
