all:
	make -C src

test:
	make -C src
	make -C src check
	make -C test

clean:
	make -C src clean
	make -C test clean

install:
	test -d $(DESTDIR)/bin
	cp -t $(DESTDIR)/bin \
	 src/asm2hack \
	 src/vm2asm \
	 src/jack2vm \
	 src/vm2hack \
	 src/emulator \
	 src/emulator-gui \
	 src/jack2ssa \
	 src/ssa2asm

.PHONY: test clean install
