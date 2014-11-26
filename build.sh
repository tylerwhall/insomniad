#!/bin/sh

set -e
set -x

git clean -dfx
autoreconf -i
./configure
make
#make check
