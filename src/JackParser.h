// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#pragma once
#include <string>
#include <stdexcept>
#include "JackAST.h"
#include "JackTokenizer.h"


namespace hcc {
namespace jack {


/**
 * An exception thrown when parser encounters error.
 */
class ParseError
    : public std::runtime_error
{
public:
    /**
     * Construct from error message and tokenizer instance.
     */
    ParseError(const std::string& what, const Tokenizer& tokenizer)
        : runtime_error(what)
        , line(tokenizer.getLine())
        , column(tokenizer.getColumn())
    {}


    virtual ~ParseError() throw () {}


    /** a column where error occured */
    unsigned line;


    /** a column where error occured */
    unsigned column;
};


/**
 * A parser for Jack language.
 */
class Parser {
public:
    Parser();
    ~Parser();

    /**
     * Attempt to parse input and return AST.
     */
    ast::Class parse(const std::string& filename);

private:
    // private implementation
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

} // end namespace jack
} // end namespace hcc
