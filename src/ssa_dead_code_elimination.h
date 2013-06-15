// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#ifndef SSA_DEAD_CODE_ELIMINATION_H
#define SSA_DEAD_CODE_ELIMINATION_H

#include "ssa.h"

namespace hcc { namespace ssa {

void dead_code_elimination(instruction_list& instructions);

}} // namespace hcc::ssa

#endif // SSA_DEAD_CODE_ELIMINATION_H
