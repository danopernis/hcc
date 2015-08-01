all:
	make -C src

test:
	make -C src
	make -C src check

clean:
	make -C src clean

install:
	test -d $(DESTDIR)/bin
	cp -t $(DESTDIR)/bin \
	 src/asm2hack \
	 src/vm2asm \
	 src/jack2vm \
	 src/vm2hack \
	 src/emulator-gui \
	 src/hcc

.PHONY: test clean install
