
\page subscriptions Subscriptions

The flow and definition of a subscription has three parts:

 - Read the current state of the entry (a file or device or any other endpoint)
 - Read the metadata of that thing
 - Subscribe to any future changes on that thing, which will come as diffs.

From these steps, a clever client can maintain state with the remote model and the user's view will be consistent with the server state.

Expect disconnections!  A disconnected client will try to reconnect, but upon reconnection it should expect to rebuild its state.  A clever client will receive the read data and metadata and compare to the current state during rebuild.  The poor man's version is to destroy the current state and always rebuild from scratch.

These concepts are critical for mobile applications, where disconnection may be frequent.

An important concept is the ability to chain subscriptions.  Diffs will cascade to subscribers in order.  User endpoints are not the only things that can subscribe to each other.

FIXME: MORE HERE... describe how the server can subscribe parts to itself.



