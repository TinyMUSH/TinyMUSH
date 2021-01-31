#!/usr/bin/env bash

# Just a quick hack for now. Need to be fixed eventually

TINYMUSH_DIR="$(pwd)"
autoreconf -ivfm
cd src/libltdl/
autoreconf -ivfm
cd $TINYMUSH_DIR
./configure && make distclean
