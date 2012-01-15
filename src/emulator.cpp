#include <iostream>
#include <fstream>
#include <cstdlib>
#include "cpu.h"

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
        std::ifstream input(argv[1]);
	if (!input.good()) {
		std::cerr << "Error: Could not open " << argv[1] << std::endl;
		return 1;
	}

	hcc::CPU cpu;
	if (!cpu.load(input)) {
		std::cerr << "Error: " << argv[1] << " is not a HACK file" << std::endl;
		return 1;
	}

	for (int i = 0; i<ticks; ++i)
		cpu.step();
	
	for (int i = 3; i<argc; ++i) {
		unsigned int address = atoi(argv[i]);
		std::cout << "RAM[" << address << "] =\t" << cpu.getRam(address) << std::endl;
	}

	return 0;
}
