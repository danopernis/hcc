all:
	../../src/vm2hack *.vm
	../../src/emulator output.hack 1000 261 3 4 3032 3046 >result
	diff result compare

clean:
	rm -f output.hack result
