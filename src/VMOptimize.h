/*
 * Copyright (c) 2012 Dano Pernis
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
#include "VMCommand.h"

namespace hcc {

/*
 * optimizeN routine calls callback for all successive N-tuples. If callback returns true, routine restarts.
 */
typedef void (*o1cb)(VMCommandList &cmds, VMCommandList::iterator &c1);
typedef bool (*o2cb)(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);
typedef bool (*o3cb)(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2, VMCommandList::iterator &c3);

void optimize1(VMCommandList &cmds, o1cb cb);
void optimize2(VMCommandList &cmds, o2cb cb);
void optimize3(VMCommandList &cmds, o3cb cb);

// optimizations removing commands
bool o_bloated_goto(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2, VMCommandList::iterator &c3);
bool o_double_notneg(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);
bool o_negated_compare(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);
bool o_negated_if(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);

// const expressions
bool o_const_expression3(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2, VMCommandList::iterator &c3);
bool o_const_expression2(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);
bool o_const_if(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);

// convert binary operation to unary
bool o_const_swap(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2, VMCommandList::iterator &c3);
bool o_binary_to_unary(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);
void o_special_unary(VMCommandList &cmds, VMCommandList::iterator &c1);
bool o_binary_equalarg(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2, VMCommandList::iterator &c3);

// merge
bool o_push_pop(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);
bool o_compare_if(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);
bool o_goto_goto(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);
bool o_pop_push(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);

// stack-less computation chains
void s_replicate(VMCommandList &cmds, VMCommandList::iterator &c1);
bool s_reduce(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);
bool s_reconstruct(VMCommandList &cmds, VMCommandList::iterator &c1, VMCommandList::iterator &c2);

void o_stat_reset();
void o_stat_print();

} // end namespace
