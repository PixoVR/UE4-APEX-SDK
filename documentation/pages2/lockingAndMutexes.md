
\page lockingAndMutexes Locking and Mutexes

When many users are logged in and simultaneously manipulating data, race conditions will exist.  Each node in the VFS is
owned and handled by a single thread.  Multithreading is achieved by putting nodes on different threads.  Diff operations are
performed atomically, and diffs are distributed to subscribers on whatever thread they exist in.

Locking is provided as a series of optional functions that need to be implemented in a VFS_node subclass.  A VFS_node will need
to record its own locks and grant them as necessary.

Make no mistake: this is tricky, to be sure.

A client may request a lock, which will either be granted or denied.  In the case that it's granted, the client must release it
when it makes sense, and the plugin needs to ensure that if the client is disconnected, the lock is released.  If the lock is
denied, the client must decide what to do.  This may be to query again, or to deny user input.

There is not a case where a blocking operation will wait until a request for a lock succeeds, as this doesn't make sense, and
creates an indeterminate state.  Most of the work is in maintaining client consistency.  Internally the VFS needs to 
always be consistent, and the design makes the need for mutexes rare.  The developer needs to consider how locking is meant
to be used, and whether it's an interface issue or something deeper.

Let's consider a few cases:

A user is trying to type data into a form field while another is doing the same.  Without locks, both will see each other's
input and potentially be frustrated, but it will be consistent.  The server will know the current state of the form field and
each user will receive and see the other's input.  This is perhaps annoying, but our goal in this basic case is to make sure the
server doesn't crash and that no data is lost.  With locks, the second user cannot modify the field owned by the first user.
This is better, and is also consistent, but requires a lock query to enter the form field in the first place.  This will be
implemented in the future.

A more difficult case is when a first user is editing a thing while a second user is deleting it.  We have a concept of field
locks, object locks, and even group locks.  This is the primary reason for leaving locks to the node and not implementing it
globally.  Locks are expensive, and need to be thought out when implemented.

A VFS_node can maintain locks using VFS_node::requestLock(VFS_request *) and VFS_node::releaseLock(VFS_request *).  This is
usually an interplay between the client and the server, but may also be between internal nodes.

