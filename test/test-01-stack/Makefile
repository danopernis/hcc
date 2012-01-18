all:
	../../src/vm2asm2hack *.vm
	../../src/emulator output.hack 1000 0 261 262 263 264 >result
	diff result compare

clean:
	rm -f output.hack result
