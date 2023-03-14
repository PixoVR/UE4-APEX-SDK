
\page server Server Installation

\section server_requirements Server Requirements

 - Qt libraries are required: https://www.qt.io/ 
 - Qt5.6+ is required for websocket support.  Please note that proper UTF support is required for JSON and BSON functions.  There is a version of Qt for Centos6 that does not have full support compiled in.
 - A C++ compiler that supports C++11 is requred.
 - A webserver like Apache or Nginx is required for browser access, which is not required by the server in any way.
 - For Linux:
    - Depending on distro, you may use the Qt online installer
    - If apt (Ubuntu):
       - qt5-default			-- main qt source files
       - libqt5websockets5-dev		-- websockets
       - qtdeclarative5-dev		-- qml/scripting
       - libqt5serialport5-dev		-- for modbus, if needed
    - If yum (Centos 7, Docker):
       - qt5-qtbase-devel.x86_64		-- main qt source files
       - qt5-qtwebsockets-devel.x86_64	-- websockets
       - qt5-qtdeclarative-devel.x86_64	-- qml/scripting
       - qt5-qtserialport-devel.x86_64 	-- for modbus, if needed
 - For OSX:
    - XCode with command line tools (you don't have to use XCode, but it will need to be installed)
    - Use the Qt installer, or build from source
 - Windows:
    - untested in all ways
 - Bash is needed for all build scripts

\sa \ref nginx
\sa \ref documentation

\section server_build Building the Executable

Clone the source code:

\code
git clone ssh://git@gitlab.eyelash.ai:2222/vfs/core/vfs_server.git
\endcode

Then build it:

\code{.sh}
cd vfs_server/
cd server/
./build.sh [mode]
\endcode

This will build libVFS and the VFS executable, as well as compile any plugins in
the src/plugins/buildPlugins.sh script.

build.sh has modes:

Mode			| Description
------------------------|--------------------------------------------------------------------------------------
build			| Build the VFS-lib.pro project, as well as VFS.pro.  This will build the executable.
buildplugins		| Call the src/plugins/buildPlugins.sh script with a mode of 'build'.
buildall		| Build the executable and the plugins
buildrun [arguments]	| Build the executable and run it with arguments
clean			| Clean the VFS-lib.pro and VFS.pro projects
cleanbuild		| Clean, then build and run
cleanbuildrun [arguments] | Clean, then build, then run with arguments
run [arguments]		| Run the executable with arguments

The default mode is 'buildall'.  The build.sh script can be run from any directory.  Any of the modes that run
the executable can receive arguments.

\section server_usage Invoking the Server

\code
VFS [[-config] config.xml] [-plugins pluginsDir] [-plugins ...] [-version] [-printconfig] [-describe] [-?] [[name=val] ...]

   -config     the path to an xml config file
   -plugins    the absolute or relative path to a directory to scan for plugins.  This flag can appear multiple times.
   -version    print version
   -describe   print a description for each loaded plugin and exit.
   -?          print this message
   name=val    an environment variable to set.  Multiple variables can be provided.
\endcode

\cond NOSHOW
 - Binary options can be specified as '-name=val' or '-name val'.
 - Unary options can be specified as '-name' or '-enable-name' or '-disable-name'.
 - All trailing pairs matching name=val are set as environment variables.
 - Multiple plugins folders can be defined and all will be scanned.
 - Plugins paths are absolute, or relative to the CWD.
 - The websocket port is defined in config.xml
\endcond
 
 The VFS executable can be run from any directory.

 \sa parseApplicationOptions()


