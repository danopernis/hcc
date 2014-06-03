// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#pragma once
#include <string>

namespace hcc {

namespace instruction {

// instruction (de)coding
static const unsigned short COMPUTE   = 1 << 15;
static const unsigned short RESERVED  = 1 << 14
				      | 1 << 13;
static const unsigned short FETCH     = 1 << 12;
static const unsigned short ALU_ZX    = 1 << 11; // zero x
static const unsigned short ALU_NX    = 1 << 10; // negate x
static const unsigned short ALU_ZY    = 1 <<  9; // zero y
static const unsigned short ALU_NY    = 1 <<  8; // negate y
static const unsigned short ALU_F     = 1 <<  7; // compute ADD if set, AND if not set
static const unsigned short ALU_NO    = 1 <<  6; // negate output
static const unsigned short DEST_A    = 1 <<  5;
static const unsigned short DEST_D    = 1 <<  4;
static const unsigned short DEST_M    = 1 <<  3;
static const unsigned short JUMP_NEG  = 1 <<  2;
static const unsigned short JUMP_ZERO = 1 <<  1;
static const unsigned short JUMP_POS  = 1 <<  0;

// shorthands
static const unsigned short JLT = JUMP_NEG;
static const unsigned short JEQ = JUMP_ZERO;
static const unsigned short JGT = JUMP_POS;
static const unsigned short JGE = JUMP_ZERO | JUMP_POS;
static const unsigned short JLE = JUMP_ZERO | JUMP_NEG;
static const unsigned short JNE = JUMP_NEG | JUMP_POS;
static const unsigned short JMP = JUMP_NEG | JUMP_ZERO | JUMP_POS;

// 28 documented operations
static const unsigned short COMP_ZERO        = ALU_ZX | ALU_ZY | ALU_F;
static const unsigned short COMP_ONE         = ALU_ZX | ALU_NX | ALU_ZY | ALU_NY | ALU_F | ALU_NO;
static const unsigned short COMP_MINUS_ONE   = ALU_ZX | ALU_NX | ALU_ZY | ALU_F;
static const unsigned short COMP_D           = ALU_ZY | ALU_NY;
static const unsigned short COMP_A           = ALU_ZX | ALU_NX;
static const unsigned short COMP_NOT_D       = ALU_ZY | ALU_NY | ALU_NO;
static const unsigned short COMP_NOT_A       = ALU_ZX | ALU_NX | ALU_NO;
static const unsigned short COMP_MINUS_D     = ALU_ZY | ALU_NY | ALU_F | ALU_NO;
static const unsigned short COMP_MINUS_A     = ALU_ZX | ALU_NX | ALU_F | ALU_NO;
static const unsigned short COMP_D_PLUS_ONE  = ALU_NX | ALU_ZY | ALU_NY | ALU_F | ALU_NO;
static const unsigned short COMP_A_PLUS_ONE  = ALU_ZX | ALU_NX | ALU_NY | ALU_F | ALU_NO;
static const unsigned short COMP_D_MINUS_ONE = ALU_ZY | ALU_NY | ALU_F;
static const unsigned short COMP_A_MINUS_ONE = ALU_ZX | ALU_NX | ALU_F;
static const unsigned short COMP_D_PLUS_A    = ALU_F;
static const unsigned short COMP_D_MINUS_A   = ALU_NX | ALU_F | ALU_NO;
static const unsigned short COMP_A_MINUS_D   = ALU_NY | ALU_F | ALU_NO;
static const unsigned short COMP_D_AND_A     = 0;
static const unsigned short COMP_D_OR_A      = ALU_NX | ALU_NY | ALU_NO;
static const unsigned short COMP_M           = FETCH | COMP_A;
static const unsigned short COMP_NOT_M       = FETCH | COMP_NOT_A;
static const unsigned short COMP_MINUS_M     = FETCH | COMP_MINUS_A;
static const unsigned short COMP_M_PLUS_ONE  = FETCH | COMP_A_PLUS_ONE;
static const unsigned short COMP_M_MINUS_ONE = FETCH | COMP_A_MINUS_ONE;
static const unsigned short COMP_D_PLUS_M    = FETCH | COMP_D_PLUS_A;
static const unsigned short COMP_D_MINUS_M   = FETCH | COMP_D_MINUS_A;
static const unsigned short COMP_M_MINUS_D   = FETCH | COMP_A_MINUS_D;
static const unsigned short COMP_D_AND_M     = FETCH | COMP_D_AND_A;
static const unsigned short COMP_D_OR_M      = FETCH | COMP_D_OR_A;

// masks
static const unsigned short MASK_COMP = FETCH | ALU_ZX | ALU_NX | ALU_ZY | ALU_NY | ALU_F | ALU_NO;
static const unsigned short MASK_DEST = DEST_A | DEST_D | DEST_M;
static const unsigned short MASK_JUMP = JUMP_NEG | JUMP_ZERO | JUMP_POS;

} // end namespace

} // end namespace
