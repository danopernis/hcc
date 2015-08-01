HCC (HACK Compiler Collection)
==============================

This is an effort to make the best available toolchain for HACK computer platform.
See <http://danopernis.github.io/hcc/> for details.
[![Build Status](https://travis-ci.org/danopernis/hcc.svg?branch=master)](https://travis-ci.org/danopernis/hcc)

Installation
------------

Build HCC using plain make

> make -C src all

Install binaries using

> DESTDIR=~ make -C src install

Standard library/OS is available in directory "lib". Contents of this directory
shall be copied to your Jack project.
