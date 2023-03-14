
\page commands Commands

These operations control the flow of data between nodes:

command	| method				| description
--------|---------------------------------------|--------------------------------------------------------
ls	| VFS_node::ls(VFS_request *) 		| list children as a series of strings/names
create	| VFS_node::write(VFS_request *)	| create a child.  Metadata will probably be needed.
rm	| VFS_node::rm(VFS_request *)		| remove a child
read	| VFS_node::read(VFS_request *)		| return the contents of a node to a destination
write	| VFS_node::write(VFS_request *)	| commit the data portion of a request to a destination
metadata | VFS_node::metadata(VFS_request *)	| return the metdata of a node
report	| VFS_node::report(VFS_request *)	| mostly for debugging.  Report whatever is interesting about this node to help a programmer.
diff	| VFS_node::applyDiff(VFS_request *)	| receive a diff.  Generally this will be merely changing values, but is open to arbitrary complexity.
submit	| VFS_node::submit(VFS_request *)	| apply a diff.  This will call diff() and cascade to subscribers.  This is in contrast to write(), which will clobber content.  Submit will merge content.
subscribe | VFS_node::subscribe(VFS_request *)	| read the data of an endpoint, as well as the metadata.  Future diffs will be sent.
unsubscribe | VFS_node::unsubscribe(VFS_request *) | stop listening
requestlock | VFS_node::requestLock(VFS_request *) | request a lock on this node.  The default behavior is undefined; it needs to be implemented in subclasses to have meaning.
releaselock | VFS_node::releaseLock(VFS_request *) | release a lock on this node.  The default behavior is undefined; it needs to be implemented in subclasses to have meaning.
code	| VFS_node::code(QString, QString, QString &) | nodes frequently come as plugins, and plugins have the option to provide code.  This makes sense in a javascript context, and may make sense in other contexts later.

The implementation of each function in a node should be thread-safe and atomic.  A node should not maintain pointers to any node in another thread, and potentially should never hold a pointer to another node at all, although there are exceptions.

All of these commands except code() receive a VFS_request * as an argument.  A VFS_request can contain both data and metadata, as well as a path.  A request contains all the data involved with an operation, and that data is copied between threads, when signals are called, etc.

\warning Large blobs of data (on the order of gigabytes) have not yet been tested.

