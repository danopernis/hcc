HCC (HACK Compiler Collection)
==============================

This is an effort to make the best available toolchain for HACK computer platform.
See <http://danopernis.github.io/hcc/> for details.

Installation
------------

Build HCC using plain make

> make

Optionally run tests

> make test

Install binaries using

> DESTDIR=~ make install

Standard library/OS is available in directory "lib". Contents of this directory
shall be copied to your Jack project.
