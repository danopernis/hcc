// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#ifndef SSA_CONSTRUCTION_H
#define SSA_CONSTRUCTION_H

#include "ssa.h"

namespace hcc { namespace ssa {

void construct_minimal_ssa(instruction_list& instructions);

}} // namespace hcc::ssa

#endif // SSA_CONSTRUCTION_H
