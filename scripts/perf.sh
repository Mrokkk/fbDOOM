#!/bin/bash

set -e

FLAMEGRAPH_DIR=".FlameGraph"

cleanup()
{
    [ -n "${OUT_PERF}" ]   && rm -rf "${OUT_PERF}"
    [ -n "${OUT_FOLDED}" ] && rm -rf "${OUT_FOLDED}"
    [ -f perf.data ]       && rm -rf perf.data
}

trap cleanup EXIT

if [ ! -d "${FLAMEGRAPH_DIR}" ]
then
    git clone --revision 41fee1f99f9276008b7cd112fca19dc3ea84ac32 --depth 1 https://github.com/brendangregg/FlameGraph "${FLAMEGRAPH_DIR}"
fi

period="${1}"
shift

OUT_PERF="$(mktemp)"
OUT_FOLDED="$(mktemp)"

perf record -F "${period}" -g -- ${@}
perf script > "${OUT_PERF}"
"${FLAMEGRAPH_DIR}/stackcollapse-perf.pl" "${OUT_PERF}" > "${OUT_FOLDED}"
"${FLAMEGRAPH_DIR}/flamegraph.pl" "${OUT_FOLDED}" > doom.svg
readlink -f doom.svg
