#!/usr/bin/env bash

# Just a quick hack for now. Need to be fixed eventually

TINYMUSH_DIR="$(pwd)"
autoreconf -ivf
cd src/libltdl/
autoreconf -ivf
cd $TINYMUSH_DIR
./configure && make distclean
