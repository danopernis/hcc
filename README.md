HCC (HACK Compiler Collection)
==============================

This is an effort to make the best available toolchain for HACK computer platform.
See <http://danopernis.github.io/hcc/> for details.
[![Build Status](https://travis-ci.org/danopernis/hcc.svg?branch=master)](https://travis-ci.org/danopernis/hcc)


Building & Installation
-----------------------

Build HCC using

> mkdir build
> cd build
> cmake ..
> make

Optionally test and install using

> make test
> DESTDIR=~ make install

Standard library/OS is available in directory "src/stdlib".
Contents of that directory shall be copied to your Jack project.
