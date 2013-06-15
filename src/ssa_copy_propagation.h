// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#ifndef SSA_COPY_PROPAGATION_H
#define SSA_COPY_PROPAGATION_H

#include "ssa.h"

namespace hcc { namespace ssa {

void copy_propagation(instruction_list& instructions);

}} // namespace hcc::ssa

#endif // SSA_COPY_PROPAGATION_H
