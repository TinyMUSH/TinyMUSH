#!/bin/sh

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    sudo apt-get -y install cproto libpcre3-dev libgdbm-dev doxygen graphviz
fi
