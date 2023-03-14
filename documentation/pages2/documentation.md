
\page documentation Build the Documentation

Building the documentation requires doxygen: http://www.doxygen.org.  It can be installed with a package manager (yum, apt-get, port, homebrew) or built from source.

Doxygen builds have been tested on versions >=1.8.13.

\note A doxygen 1.8.13 binary is provided for linux in the docker folder, and is used during containerization.

Once doxygen is installed:

    cd documentation/doxygen
    ./build.sh

The html version can then be accessed at:

    http://[domain]/[vfs]/documentation/html

