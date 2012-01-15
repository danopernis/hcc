all:
	../../src/hcc-vm *.vm
	../../src/hcc-asm output.asm
	../../src/hcc-emulator output.hack 1000 0 261 262 263 264 >result
	diff result compare

clean:
	rm -f output.asm output.hack result
