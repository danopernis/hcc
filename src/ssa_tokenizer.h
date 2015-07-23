// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#pragma once
#include <iostream>
#include <string>

namespace hcc {
namespace ssa {

enum class token_type {
    CONSTANT,
    REG,
    GLOBAL,
    LOCAL,
    LABEL,
    DEFINE,
    BLOCK,
    RETURN,
    JUMP,
    JLT,
    JEQ,
    CALL,
    ARGUMENT,
    LOAD,
    STORE,
    MOV,
    ADD,
    SUB,
    AND,
    OR,
    NEG,
    NOT,
    PHI,
    SEMICOLON,
    END,
};

std::string to_string(const token_type&);

struct token {
    token_type type;
    std::string token;
    unsigned line_number;
};

struct tokenizer {
    tokenizer(std::istream& input) : current {input} { }

    token next();

private:
    std::istreambuf_iterator<char> current;
    unsigned line_number {1};
};

} // namespace ssa
} // namespace hcc
