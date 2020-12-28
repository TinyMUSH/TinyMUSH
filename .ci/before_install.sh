#!/bin/sh

cd $TRAVIS_BUILD_DIR
autoreconf -ivf
cd $TRAVIS_BUILD_DIR/src/libltdl
autoreconf -ivf
cd $TRAVIS_BUILD_DIR
./configure && make distclean
