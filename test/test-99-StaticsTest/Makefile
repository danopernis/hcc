all:
	../../src/hcc-vm2hack *.vm
	../../src/hcc-emulator output.hack 2500 0 261 262 >result
	diff result compare

clean:
	rm -f output.hack result
