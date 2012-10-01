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
