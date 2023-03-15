#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/doxygen

source ../env.sh

# may want to add this someday...
# http://coherent-labs.com/blog/documenting-javascript-with-doxygen/

# added unreal filter, but renamed the script
# https://github.com/normalvector/ue4_doxygen_source_filter

rm -rf ../html

doxygen doxyfile

cp -rv images ../html/images

cd ../

