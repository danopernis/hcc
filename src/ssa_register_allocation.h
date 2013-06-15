// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#ifndef SSA_REGISTER_ALLOCATION_H
#define SSA_REGISTER_ALLOCATION_H

#include "ssa.h"

namespace hcc { namespace ssa {

void allocate_registers(instruction_list& instructions);

}} // namespace hcc::ssa

#endif // SSA_REGISTER_ALLOCATION_H
