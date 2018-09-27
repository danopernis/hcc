// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details

#pragma once
#include "JackAST.h"

namespace hcc {
namespace jack {

class VMWriter {
public:
    VMWriter(const std::string& filename);
    ~VMWriter();
    void write(const ast::Class& clazz);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace jack {
} // namespace hcc {
