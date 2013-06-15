// Copyright (c) 2014 Dano Pernis
// See LICENSE for details

#ifndef SSA_ADHOC_H
#define SSA_ADHOC_H

#include "ssa.h"

namespace hcc { namespace ssa {

void prettify_names(instruction_list& instructions, unsigned& var_counter, unsigned& label_counter);
void clean_cfg(instruction_list& instructions);

}} // namespace hcc::ssa

#endif // SSA_ADHOC_H
