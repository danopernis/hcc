#!/bin/bash
# Copyright (c) 2012-2018 Dano Pernis

(
echo "digraph depend {"
for file1 in *.{cc,h}; do
    if `echo -n $file1 | grep -q '\.cc$'`; then
        echo "\"$file1\" [style=filled, fillcolor=gray];"
    fi
    for file2 in `grep '^#include "' $file1 |cut -d '"' -f 2`; do
        echo "\"$file1\" -> \"$file2\";"
    done
done
echo "}"
) | dot -Tpng >include_graph.png
