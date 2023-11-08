#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

# exit on errors
set -e

#wget https://gitlab.com/api/v4/projects/4207231/packages/generic/graphviz-releases/8.0.5/graphviz-8.0.5.tar.gz
#tar -zxf graphviz-8.0.5.tar.gz

git clone --recurse-submodules https://gitlab.com/graphviz/graphviz.git
cd graphviz
#git lfs fetch

#mkdir build
#cd build
#cmake ../

./autogen.sh
./configure --enable-static

make -j 8
make install

#cp ./cmd/dot/dot /usr/local/bin/dot

# cleanup
cd $DIR
rm -rf graphviz

dot -V


