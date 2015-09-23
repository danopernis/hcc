// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#include "ssa.h"
#include <iostream>
#include <vector>

namespace {

using hcc::ssa::argument;
using hcc::ssa::constant;
using hcc::ssa::instruction;
using hcc::ssa::instruction_type;
using hcc::ssa::reg;
using hcc::ssa::subroutine;

struct ssa_worklist_entry {
};

enum class value_type {
    TOP,
    CONSTANT,
    BOTTOM,
};

struct value {
    value_type type {value_type::TOP};
    constant value {0};
};

struct analyzer {
    analyzer(subroutine& s)
    {
        cfg_worklist.push_back(s.entry_node().name);

        value BOTTOM;
        BOTTOM.type = value_type::BOTTOM;

        while (!cfg_worklist.empty() || !ssa_worklist.empty()) {
            if (!cfg_worklist.empty()) {
                const auto label = cfg_worklist.back();
                cfg_worklist.pop_back();

                std::cout << label.save_fast() << "\n";
                auto& bb = s.basic_blocks.at(label);
                bb.work = 1;

                for (const auto& i : bb.instructions) {
                    std::cout << i.save_fast() << "\n";
                    switch (i.type) {
                    case instruction_type::JUMP:
                        cfg_worklist.push_back(i.arguments[0].get_label());
                        break;
                    case instruction_type::JEQ:
                        evaluate_branch(i, [] (const constant& c) { return c.value == 0; });
                        break;
                    case instruction_type::JLT:
                        evaluate_branch(i, [] (const constant& c) { return c.value <  0; });
                        break;
                    case instruction_type::ARGUMENT:
                        set_value(i.arguments[0].get_reg(), BOTTOM);
                        break;
                    case instruction_type::RETURN:
                        // nothing to do, as this is not a definition
                        break;
                    case instruction_type::NEG:
                        set_value(
                            i.arguments[0].get_reg(),
                            evaluate(i.type, get_value(i.arguments[1])));
                        break;
                    case instruction_type::AND:
                    case instruction_type::OR:
                        set_value(
                            i.arguments[0].get_reg(),
                            evaluate(i.type, get_value(i.arguments[1]), get_value(i.arguments[2])));
                        break;
                    default:
                        assert (false && "not implemented");
                    }
                }
            }
            if (!ssa_worklist.empty()) {
                const auto item = ssa_worklist.back();
                ssa_worklist.pop_back();
                assert (false && "not implemented");
            }
        }
    }

private:
    value get_value(const argument& a)
    {
        if (a.is_constant()) {
            value v;
            v.type = value_type::CONSTANT;
            v.value = a.get_constant();
            return v;
        } else if (a.is_reg()) {
            return values.at(a.get_reg());
        }
        assert (false && "not implemented");
    }

    value evaluate(instruction_type type, const value& a, const value& b)
    {
        switch (type) {
        case instruction_type::SUB:
            if (a.type == value_type::BOTTOM || b.type == value_type::BOTTOM) {
                value v;
                v.type = value_type::BOTTOM;
                return v;
            }
            if (a.type == value_type::CONSTANT && b.type == value_type::CONSTANT) {
                value v;
                v.type = value_type::CONSTANT;
                v.value.value = a.value.value - b.value.value;
                return v;
            }
            assert (false && "not implemented");
            break;
        case instruction_type::AND:
            if (a.type == value_type::CONSTANT && b.type == value_type::CONSTANT) {
                value v;
                v.type = value_type::CONSTANT;
                v.value.value = a.value.value & b.value.value;
                return v;
            }
            assert (false && "not implemented");
            break;
        case instruction_type::OR:
            if (a.type == value_type::CONSTANT && b.type == value_type::CONSTANT) {
                value v;
                v.type = value_type::CONSTANT;
                v.value.value = a.value.value | b.value.value;
                return v;
            }
            assert (false && "not implemented");
            break;
        default:
            assert (false && "not implemented");
        };
        assert (false && "not implemented");
    }

    value evaluate(instruction_type type, const value& a)
    {
        switch (type) {
        case instruction_type::NEG:
            if (a.type == value_type::BOTTOM) {
                return a;
            }
            assert (false && "not implemented");
            break;
        default:
            assert (false && "not implemented");
        };
        assert (false && "not implemented");
    }

    void set_value(const reg& r, value v)
    {
        values[r] = v;
    }

    template<typename F>
    void evaluate_branch(const instruction& i, F&& f)
    {
        const auto positive = i.arguments[2].get_label();
        const auto negative = i.arguments[3].get_label();
        const auto result = evaluate(
            instruction_type::SUB,
            get_value(i.arguments[0]),
            get_value(i.arguments[1]));

        switch (result.type) {
        case value_type::TOP:
            assert(false && "branch on uninitialized value");
        case value_type::CONSTANT:
            if (std::forward<F>(f)(result.value)) {
                cfg_worklist.push_back(positive);
            } else {
                cfg_worklist.push_back(negative);
            }
            break;
        case value_type::BOTTOM:
            cfg_worklist.push_back(positive);
            cfg_worklist.push_back(negative);
            break;
        }
    }

    std::vector<hcc::ssa::label> cfg_worklist;
    std::vector<ssa_worklist_entry> ssa_worklist;

public:
    std::map<reg, value> values;
};

} // anonymous namespace

namespace hcc { namespace ssa {

void subroutine::sccp()
{
    analyzer an {*this};

    for (const auto& kv : an.values) {
        std::cout << kv.first.save_fast() << ": ";
        switch (kv.second.type) {
        case value_type::BOTTOM:
            std::cout << "bottom\n";
            break;
        case value_type::CONSTANT:
            std::cout << "constant " << kv.second.value.save_fast() << "\n";
            break;
        case value_type::TOP:
            std::cout << "top\n";
            break;
        }
    }

    for_each_bb([&] (basic_block& bb) {
        for (auto& i : bb.instructions) {
            i.use_apply([&] (argument& a) {
                if (!a.is_reg()) {
                    return;
                }
                auto it = an.values.find(a.get_reg());
                if (it == an.values.end()) {
                    return;
                }
                const auto& v = it->second;
                if (v.type != value_type::CONSTANT) {
                    return;
                }
                a = v.value;
            });
        }
    });
}

}} // end namespace hcc::ssa
