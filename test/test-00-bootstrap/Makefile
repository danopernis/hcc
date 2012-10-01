all:
	../../src/vm2asm *.vm
	../../src/asm2hack output.asm
	../../src/emulator output.hack 1000 0 1 2 >result
	diff result compare
	rm -f output.hack
	../../src/vm2hack *.vm
	../../src/emulator output.hack 1000 0 1 2 >result
	diff result compare

clean:
	rm -f output.asm output.hack result
