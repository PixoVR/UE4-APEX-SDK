
\page nodesAndPlugins Nodes and Plugins

All nodes in VFS must be a subclass of VFS_node.  The base VFS_node class only acts as a child and/or parent in the VFS tree, with the ability to subscribe or submit to other nodes in the tree.

Nodes will want to override the methods described in \ref commands.

\section nodesAndPlugins_plugins Plugins

Plugins provide a bundle of possible nodes to the VFS.  The VFS_node_interface is the base class for all
plugins.

FIXME: explain plugins and commands

FIXME: add form fields here

\sa VFS_node_interface
\sa \ref server_usage

\section nodesAndPlugins_storage Storage

 - VFS_HD
 - VFS_QRC
 - VFS_mongo
 - VFS_SQL node (future)
 - VFS_S3 (future)

\section nodesAndPlugins_networking Networking

 - VFS_httpd (http(s) requests)
 - VFS_remotoserver (websockets)
 - VFS_tcp_server
 - VFS_tcp_client

\section nodesAndPlugins_skeleton The Plugin Skeleton

\ref example_skeleton


