// Copyright (c) 2012-2018 Dano Pernis
// See LICENSE for details
#pragma once

#include <string>
#include <memory>

namespace hcc {
namespace jack {

struct Class;

class VMWriter {
public:
    VMWriter(const std::string& filename);
    ~VMWriter();
    void write(const Class& clazz);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace jack {
} // namespace hcc {
