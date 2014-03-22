// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#pragma once
#include "JackParser.h"

namespace hcc {
namespace jack {

class VMWriter {
public:
    VMWriter(const std::string& filename);
    ~VMWriter();
    void write(const ast::Class& clazz);
private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

} // end namespace jack
} // end namespace hcc
