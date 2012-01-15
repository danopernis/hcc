all:
	../../src/hcc-vm2hack *.vm
	../../src/hcc-emulator output.hack 6000 0 261 >result
	diff result compare

clean:
	rm -f output.hack result
