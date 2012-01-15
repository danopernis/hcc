all:
	../../src/hcc-vm *.vm
	../../src/hcc-asm output.asm
	../../src/hcc-emulator output.hack 2500 0 261 262 >result
	diff result compare

clean:
	rm -f output.asm output.hack result
