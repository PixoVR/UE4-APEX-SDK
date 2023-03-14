
\mainpage The Virtual File System

The VFS is a very generic engine for connecting sources to destinations.  It uses plugins to achieve this, as each source
and destination will have specific needs.  It follows a pubsub model, where a subscriber will receive a current state, and
receive diffs as state changes.  Items in the VFS will be positioned on a filesystem path.  Often times the VFS will subscribe
parts of it to itself.

`VFS` is the main server daemon. Its job is to service requests from endpoints
and maintain synchronization and consistency with subscribers.  It is the responsibility of the developer to create those
meaningful endpoints and implement the connections between them.  An input from an endpoint can pass through many parts before 
arriving as an output.

It is called a Virtual File System because nodes are related to each other hierarchically as parents and named children.
Paths are used to address nodes.  "File System" implies hierarchy, and the pathed naming scheme is used for permissions,
grouping and listing resources.  Interesting interactions happen when connecting intelligent parts of the system together
using \ref subscriptions.  For instance, a VFS_remotoserver_client is a websocket connection mounted in the filesystem.
It allows a web browser to speak to the server and a connected client will subscribe, read, and write to parts of the virtual
filesystem to interact with it.

VFS_request objects are sent from a node to a path, and from this construct we gain multithreading and multi-machine interaction,
a unified address space, and a message passing ystem that can span a large network.  This may be deceptively simple.  A
VFS_request will persist across networks, machines, threads, plugins, etc.

These subpages explain more:

 - \subpage philosophy
 - \subpage installation
 - \subpage concepts
 - \subpage examples
 - \subpage todo

In practice, the VFS executable interprets \ref configfiles and creates a daemon process for servicing requests.
