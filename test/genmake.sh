#! /bin/bash
makefile="Makefile"
echo "all:" >$makefile
for test in test-*; do
	echo -e "\tmake -C $test" >>$makefile
done
echo "clean:" >>$makefile
for test in test-*; do
	echo -e "\tmake -C $test clean" >>$makefile
done
