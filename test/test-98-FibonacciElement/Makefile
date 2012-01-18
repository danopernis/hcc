all:
	../../src/vm2asm2hack *.vm
	../../src/emulator output.hack 6000 0 261 >result
	diff result compare

clean:
	rm -f output.hack result
