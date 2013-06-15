all:
	../../src/jack2ssa *.jack >output.ssa 2>/dev/null
	../../src/ssa2asm *.ssa >output.asm 2>/dev/null
	../../src/asm2hack *.asm >output.hack 2>/dev/null
	../../src/emulator output.hack 1000 `seq 16 29` >result
	diff result compare

clean:
	rm -f output.ssa output.asm output.hack result
