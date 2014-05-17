#!/bin/sh

set -e

autoreconf -vi .
./configure
make
