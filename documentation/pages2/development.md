
\page development Development Tools

\section development_httpd HTTPD development tool

If specified in the config file, a VFS_httpd_browser node can be created, which will expose a diagnostic web page for debugging
subscriptions, threads, and browsing the tree.  This is useful for seeing the current state of the tree and also reading whatever
each node chooses to report about its current state.  By moving the mouse over a thread address, one can see which nodes are grouped
on a thread.  Similarly, a mouse over of an expanded report will highlight subscriptions.

Additionally, a node can choose to report additional information by overriding the VFS_node::reportDetails() function.  This is
especially useful for dynamic nodes and states.

\section development_plugins Plugins

Plugins should not be added to this source tree.  The ones that are there are documented as examples.  The preferred method for creating
plugins is to place them in some other location, compile them into a folder, and to specify that folder using the -plugins option to
the %VFS executable.

Depending on future development, some useful base classes may be placed there for subclassing and examples.

\section development_debuggingJS Debugging Techniques for Javascript code

In an effort to create plugins that fully encapsulate all of a node's functionality, images, code, css, etc can be embedded into a
compiled node.  This is a great feature, as a single .dylib or .so file can be distributed to an end user and that will include all
required graphics and code without having to maintain lots of files.  However, it can make things more difficult for a developer, who
will have to recompile a plugin repeatedly and restart the server each time updates are made.

The \ref example_externalPages page outlines a method of including javascript files that are not compiled in.

For development you will probably want the \ref VFS_creator::code "VFS_NOJSMIN" environment variable to be set.  Because javascript is
added without using an actual \<script\> tag, debugging syntax errors can be difficult, and minified code makes it more difficult.


