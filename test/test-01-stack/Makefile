all:
	../../src/hcc-vm2hack *.vm
	../../src/hcc-emulator output.hack 1000 0 261 262 263 264 >result
	diff result compare

clean:
	rm -f output.hack result
