/*
 * Copyright (c) 2012-2013 Dano Pernis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
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
