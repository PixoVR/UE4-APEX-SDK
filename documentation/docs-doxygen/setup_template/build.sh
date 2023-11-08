#!/bin/bash

# cd into the doxygen directory (so we know where everything else is)
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/docs-doxygen

# get the env into this shell
source ../env.sh

# pre-build our config file page
echo > ../pages/generated_configs.dox
scripts/create_configs_doc.bash pages/generated_configs.dox

# clear out any previous build
rm -rf ../html

# actually do the build
doxygen Doxyfile


