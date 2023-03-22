#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/doxygen

source ../env.sh

rm -rf ../html

doxygen doxyfile

cp -rv images ../html/images
mv ../html/images/favicon/* ../html/
rmdir ../html/images/favicon

cd ../

