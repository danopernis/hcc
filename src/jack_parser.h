// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#pragma once
#include <stdexcept>
#include "JackAST.h"

namespace hcc {
namespace jack {

struct tokenizer;

/** Parse a program in Jack language */
ast::Class parse(tokenizer&);

/** Thrown when parse() encounters an error */
struct parse_error : public std::runtime_error {
    parse_error(const std::string& what, unsigned line, unsigned column)
        : runtime_error(what)
        , line(line)
        , column(column)
    {
    }

    /** a column where error occured */
    unsigned line;

    /** a column where error occured */
    unsigned column;
};

} // end namespace jack
} // end namespace hcc
