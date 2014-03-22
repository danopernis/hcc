// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#pragma once
#include "VMCommand.h"

namespace hcc {

void o_stat_reset();
void o_stat_print();

void VMOptimize(VMCommandList &cmds);

} // end namespace
