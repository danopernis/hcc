#! /bin/sh
#
# Simple manual test.
#
# First, run
#   ./test.sh store
# to store "correct" result. Then modify binaries and run
#   ./test.sh check
# to compare the result against stored result.
#

STOREDIR=".teststore"

usage()
{
    echo "Usage: ./test {store | check}"
}

store()
{
    rm -rf lib/*.vm ${STOREDIR}
    (cd lib; ../src/jack2vm *.jack)
    mkdir -p ${STOREDIR}
    mv lib/*.vm ${STOREDIR}
}

check()
{
    rm -rf lib/*.vm
    (cd lib; ../src/jack2vm *.jack)

    for file in ${STOREDIR}/*; do
        bfile=`basename ${file}`
        if diff "${STOREDIR}/${bfile}" "lib/${bfile}" -y; then
            : #noop
        else
            echo "${bfile} is not OK"
            return
        fi
    done
    echo "Everything is OK"
}

if test $# -ne 1; then
    usage
    exit 1
fi

case $1 in
    store)
        store
        ;;
    check)
        check
        ;;
    *)
        usage
        exit 1
esac
