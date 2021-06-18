#!/bin/sh

set -e

case "$1" in
"")
    gcc fft.c stft.c wavdev.c ffeq.c -D_TEST_ -Wall -lwinmm -o ffeq
    ;;
clean)
    rm -rf *.exe
    ;;
esac
