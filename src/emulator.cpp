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
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdexcept>
#include "CPU.h"
#include "ROM.h"

struct RAM : public hcc::IRAM {
	static const unsigned int size = 0x6001;
	unsigned short *data;
public:
	RAM() {
		data = new unsigned short[size];
	}
	virtual ~RAM() {
		delete[] data;
	}
	virtual void set(unsigned int address, unsigned short value) {
		if (address >= size) {
			throw std::runtime_error("RAM::set");
		}
		
		data[address] = value;
	}
	virtual unsigned short get(unsigned int address) const {
		if (address >= size) {
			throw std::runtime_error("RAM::get");
		}
		
		return data[address];
	}
};

int main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cerr << "Usage: " << argv[0] << " PROGRAM COUNT [ADDRESS]...\n"
		             "Emulates PROGRAM running on a HACK compatible computer. After COUNT tics\n"
			     "the contents of RAM at ADDRESS are printed." << std::endl;
		return 1;
	}
	int ticks = atoi(argv[2]);
	if (ticks <= 0) {
		std::cerr << "Error: Invalid amount of ticks" << std::endl;
		return 1;
	}

	hcc::CPU cpu;
	RAM ram;
	hcc::ROM rom;
	if (!rom.load(argv[1])) {
		std::cerr << "Error: " << argv[1] << " is not a HACK file" << std::endl;
		return 1;
	}

	cpu.reset();
	for (int i = 0; i<ticks; ++i)
		cpu.step(&rom, &ram);
	
	for (int i = 3; i<argc; ++i) {
		unsigned int address = atoi(argv[i]);
		std::cout << "RAM[" << address << "] =\t" << ram.get(address) << std::endl;
	}

	return 0;
}
