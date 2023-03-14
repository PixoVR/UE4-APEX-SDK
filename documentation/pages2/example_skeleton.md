
\page example_skeleton The Skeleton Plugin

The skeleton plugin represents the most minimal amount of code needed to write a
VFS plugin.  It is included in the documentation as a starting place for new
node packages and to show how a plugin identifies itself, names itself, and is
compiled.

A plugin is composed of 3 main parts:

 - A .pro file for qmake, which will tell Qt/qmake to compile it as a plugin, which
   files are to be compiled, and what to name the resulting dynamic/shared library.
 - A VFS_node_interface subclass, which will register itself as a plugin with
   the VFS executable and Qt plugin loader, and provide an entry point for the
   node bundling mechanism in VFS_creator.
 - The actual VFS_node subclasses themselves, which will usually be the bulk of a plugin.
 
Optionally, a plugin may include .qrc resources for images, code, stylesheets, or
any other resource, which is visited in the \ref example_clock example.

For this example, we will look at this directory structure for a plugin:

\code
/skeleton.pro
/src/skeletonPlugin.h
/src/skeletonPlugin.cpp
/src/skeleton.h
/src/skeleton.cpp
\endcode

This is the minumum amount of code and apparatus required to create a plugin.

\attention The complete skeleton plugin can be found in the 'server/src/plugins/skeleton/'
folder.  If you intend to build it, you should copy it to a work folder.

\section example_skeleton_pro The skeleton.pro file

This file will be used with qmake to build the makefiles and ultimately compile the library.

\include skeleton/skeleton.pro

\attention You will want to rename your plugin to something other than "skeleton", and
you will want to include your own (renamed) source files.

\note if using Qt Creator, you will need to set the VFS_DIR and VFS_PLUGINS_DIR environment
variables. This can be done by either setting them on the terminal and opening Creator on
the command line, or by opening the project and adding VFS_DIR and VFS_PLUGINS_DIR as
variables in the __Build Environment__ section of the project settings.  Paths are relative to
the skeleton.pro file.

\note VFS_DIR must be set to the directory containing src/ in the server source tree.

\note VFS_PLUGINS_DIR must be set to the destination folder to place the compiled dynamic
library.  This variable is referenced in the VFS-plugin.pri include file.

\section example_skeletonPlugin_h The VFS_node_interface subclass header

\include skeleton/src/skeletonPlugin.h

Define the VFS_node_interface class, and identify it as a Qt plugin by providing a metadata
identifier with the Q_PLUGIN_METADATA(name) macro.  Any string can be used here, but the
example shows the common format.

Use the Q_INTERFACES() macro to tell the compiler what functions to expose in the dynamic
library.  This should always be VFS_node_interface.

\note VFS_node_interface contains pure virtual methods, so %initialize(), %description(), %licensed(), and %code()
are optional, but %provides() and %create() not optional.

\attention You will want to rename the class and the metadata to your plugin name, and don't
forget the preprocessor defines.

\sa VFS_node_interface

\section example_skeletonPlugin_cpp The VFS_node_interface subclass source

\include skeleton/src/skeletonPlugin.cpp

The plugin bundle functions need to service requests for every node type defined in
VFS_node_interface::provides().  The logic in each method is up to the developer.  It can be complex
or simple depending on your needs.  For instance, If the plugin is always licensed, licensed() could
just return `true`.

\sa VFS_node_interface

\section example_skeletonNode The VFS_node subclass implementation

Since our skeletonPlugin serves the "skeleton" node, we have to define it.  In this case, we define
a node that does nothing but construct and destruct.

Header:
\include skeleton/src/skeleton.h

Source:
\include skeleton/src/skeleton.cpp

\sa VFS_node

\note If your plugin is relatively small, all of this (both plugin and node) could be included in
single source/header files.  But generally, plugins tend to swell over time as additional functionality
is added.  It can be difficult to separate all the files later, so it is recommended to follow the
directory structure above.

\attention Much of the setup work when using the skeleton as a starting place is in replacing
"skeleton" with the proper name of your plugin.  It is recommended you use `"grep -i skeleton *"` to
find all instances and replace them to avoid future name collisions.

\section example_skeleton_build Building and Using the Plugin

If everything is configured properly, building the plugin is straightforward:

\code{.sh}
export VFS_DIR=[path/to/vfs_server/server]
export VFS_PLUGINS_DIR=[path/to/plugins/dir]
qmake skeleton.pro && make
\endcode

Now that the plugin is successfully compiled as a dynamic library, it can be included in a
\ref configfiles "Config File" to be created when the application launches.  Because provides() returns
"skeleton" as the name, we will need to use that as an entry in the config file.

\code{.xml}

//skeleton.xml

<vfs>
   <skeleton name='yorick' alpha='human' beta='120' />
</vfs>

\endcode

Running the exectuable with this config file will create a VFS that uses this plugin, and will print
a warning message when the node is created (or destroyed):

\code{.sh}
VFS skeleton.xml -plugins $VFS_PLUGINS_DIR
\endcode

This VFS is not very interesting because we have no way to talk to it and it doesn't do anything useful.
The \ref example_logview and \ref example_clock examples will take things a few steps further.

\sa \ref configfiles
\sa \ref server_usage
