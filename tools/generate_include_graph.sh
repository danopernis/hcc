#!/bin/bash
# Copyright (c) 2012-2018 Dano Pernis

DIRS=`find src -type d`
echo $DIRS
(
echo "digraph depend {"
echo "  graph [pad=1,ranksep=2];"
for DIR in $DIRS; do
    echo "  subgraph \"cluster_$DIR\" {"
    echo "    label=\"$DIR\";"
    echo "    color=gray;"
    for FILE in $DIR/*.cc; do
        test -e $FILE && echo "   \"$FILE\" [style=filled,fillcolor=gray,label=\"`basename $FILE`\"];"
    done
    for FILE in $DIR/*.h; do
        test -e $FILE && echo "    \"$FILE\" [label=\"`basename $FILE`\"];"
    done
    echo "  }"
done
for DIR in $DIRS; do
    for file1 in $DIR/*.{cc,h}; do
        test -e $file1 || continue
        for file2 in `grep '^#include "' $file1 |cut -d '"' -f 2`; do
            echo "  \"$file1\" -> \"src/$file2\";"
        done
    done
done
echo "}"
) | dot -Tpng >include_graph.png
