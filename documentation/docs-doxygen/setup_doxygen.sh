#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

# exit on errors
set -e

git clone --recurse-submodules https://github.com/doxygen/doxygen.git
cd doxygen

mkdir build
cd build
cmake -G "Unix Makefiles" ..
#make
#make -j		# unstable on some configurations!  If it runs out of memory, jobs will crash.
make -j 8

#ln -s $PWD/bin/doxygen /usr/local/bin/doxygen
cp bin/doxygen /usr/local/bin/doxygen


# delete build folder
cd $DIR
rm -rf doxygen

echo "Doxygen version: `doxygen -v`"

# because windows fileshares don't always keep permissions
cd $DIR
chmod 755 scripts/* 


