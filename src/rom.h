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
#include <fstream>
#include "cpu.h"

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
			std::cerr << "requested memory at " << address << std::endl;
			throw std::runtime_error("Memory::get");
		}
	}
};

} // end namespace
