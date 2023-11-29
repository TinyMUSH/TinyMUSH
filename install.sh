#!/bin/bash

echo -e "\nTinyMUSH version 4.0 build script"
echo -e "---------------------------------\n"
echo -e "Copyright (C) TinyMUSH development team."
echo -e "https://github.com/TinyMUSH\n"

echo -e "This script will build and install TinyMUSH in $PWD/game"
echo -e "Any existing files in $PWD/game will be deleted.\n"
read -p "Do you want to proceed? (yes/no) " yn

case $yn in
    [Yy]* )
        echo -e "\n-- Configuring build environment"
        rm -rf ./game
        cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S$PWD -B$PWD/build -G "Unix Makefiles"
        cmake --build $PWD/build --config Debug --target all -j $(getconf _NPROCESSORS_ONLN) --
        cmake --install $PWD/build
        echo -e "-- Done: Cleaning up"
        rm -rf $PWD/build
        echo -e "\nTinyMUSH has been installed in $PWD/game\n";;
    [Nn]* ) exit;;
    * ) echo "Please answer yes or no.";;
esac
