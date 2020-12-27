#!/bin/bash

# Just a quick hack for now. Need to be fixed eventually

TINYMUSH_DIR="$(pwd)"
autoreconf -ivf
cd src/libltdl/
echo $(pwd)
autoreconf -ivf
cd $TINYMUSH_DIR
./configure && make distclean
echo $(pwd)

