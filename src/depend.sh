#!/bin/sh
CXX=$1
shift
${CXX} -MM -MG "$@" | sed -e 's@^\(.*\)\.o:@\1.d \1.o:@'
