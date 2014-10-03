#!/bin/sh

set -e
set -x

autoreconf -i
./configure
make
#make check
