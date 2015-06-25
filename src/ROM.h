// Copyright (c) 2012-2015 Dano Pernis
// See LICENSE for details

#pragma once
#include <fstream>
#include "CPU.h"

namespace hcc {

struct ROM : public IROM {
	unsigned short *data;
	static const unsigned int size = 0x8000;
	ROM() {
		data = new unsigned short[size];
	}
	virtual ~ROM() {
		delete[] data;
	}

	bool load(const char *filename) {
		std::ifstream input(filename);
		std::string line;
		unsigned int counter = 0;
		while (input.good() && counter < size) {
			getline(input, line);
			if (line.size() == 0)
				continue;
			if (line.size() != 16)
				return false;

			unsigned int instruction = 0;
			for (unsigned int i = 0; i<16; ++i) {
				instruction <<= 1;
				switch (line[i]) {
				case '0':
					break;
				case '1':
					instruction |= 1;
					break;
				default:
					return false;
				}
			}
			data[counter++] = instruction;
		}
		// clear the rest
		while (counter < size) {
			data[counter++] = 0;
		}

		return true;
	}
	virtual unsigned short get(unsigned int address) const {
		if (address < size) {
			return data[address];
		} else {
			std::cerr << "requested memory at " << address << '\n';
			throw std::runtime_error("Memory::get");
		}
	}
};

} // end namespace
