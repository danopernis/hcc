all:
	../../src/vm2asm2hack *.vm
	../../src/emulator output.hack 1000 261 >result
	diff result compare

clean:
	rm -f output.hack result
