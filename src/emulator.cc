// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

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
			     "the contents of RAM at ADDRESS are printed.\n";
		return 1;
	}
	int ticks = atoi(argv[2]);
	if (ticks <= 0) {
		std::cerr << "Error: Invalid amount of ticks\n";
		return 1;
	}

	hcc::CPU cpu;
	RAM ram;
	hcc::ROM rom;
	if (!rom.load(argv[1])) {
		std::cerr << "Error: " << argv[1] << " is not a HACK file\n";
		return 1;
	}

	cpu.reset();
	for (int i = 0; i<ticks; ++i)
		cpu.step(&rom, &ram);

	for (int i = 3; i<argc; ++i) {
		unsigned int address = atoi(argv[i]);
		std::cout << "RAM[" << address << "] =\t" << ram.get(address) << '\n';
	}

	return 0;
}
