// Copyright (c) 2012-2014 Dano Pernis
// See LICENSE for details

#pragma once
#include "Assembler.h"
#include <fstream>

namespace hcc {

struct VMOutput {
	virtual ~VMOutput() {}

	virtual void emitC(unsigned short instr) = 0;
	virtual void emitA(const std::string symbol) = 0;
	virtual void emitA(unsigned short constant) = 0;
	virtual void emitL(const std::string label) = 0;
};

struct VMFileOutput : public VMOutput {
	std::ofstream stream;

	VMFileOutput(const char *file);
	virtual ~VMFileOutput() {}

	virtual void emitC(unsigned short instr);
	virtual void emitA(const std::string symbol);
	virtual void emitA(unsigned short constant);
	virtual void emitL(const std::string label);
};

struct VMAsmOutput : public VMOutput {
	AsmCommandList asmCommands;

	virtual ~VMAsmOutput();

	virtual void emitC(unsigned short instr);
	virtual void emitA(const std::string symbol);
	virtual void emitA(unsigned short constant);
	virtual void emitL(const std::string label);
};

}
