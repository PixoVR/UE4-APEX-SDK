
\page requests Requests

VFS_request instances will flow between nodes and have a particular lifespan.  We don't want to leak memory, so special care
is taken to ensure that requests are always deleted when their function has been performed.

This describes the lifespan of a request and notes about each stage of its life:

Function											| Thread	| Subclass-able	| Call Base 	| Notes
------------------------------------------------------------------------------------------------|---------------|---------------|---------------|-----------------------------------------------------------------------
VFS_node::createRequest()									| 0		| Y		| Y		| origin is the creator of the request, who wants to receive a response
VFS_node::issueRequest()									| 0		| N		| N		| on this thread and check global mutex for safety
VFS_node::subtreeRequest()									| 0->1		| N		| N		| pass to another thread
&nbsp;&nbsp;&nbsp;VFS_node::find()								| 1		| Y		| maybe		| traverse to node or fail
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Fail and VFS_node::issueResponse()				| 1		| Y		| N		| if the path is not found send an error
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Succeed and VFS_node::executeRequest()			| 1		| N		| N		| call an action using the payload in the VFS_request
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::ls()				| 1		| Y		| maybe		| return the listing of container
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::rm()				| 1		| Y		| N		| remove/delete an entry.  Won't delete a container that has children.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::read()				| 1		| Y		| N		| \copybrief VFS_node::read(VFS_request\*)
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::write()				| 1		| Y		| N		| replace or create the contents/metadata of an entry
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::metadata()			| 1		| Y		| Y		| read the metadata of entry
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::applyDiff()			| 1		| Y		| maybe		| receive a diff... __only call the base class if the subclass does not emit a diff on its own__
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::submit()			| 1		| Y		| maybe		| submit a diff to an entry
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::subscribe()			| 1		| Y		| Y		| subscribe to an entry or container.  This is tricky and not recommended for subclasses.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::unsubscribe()			| 1		| Y		| Y		| unsubscribe from an entry or container.  This is tricky and not recommended for subclasses.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::aclDefaults()			| 1		| Y		| maybe		| return any VFS_acl default settings for this node.  The ACL file has the option to override these.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::report()			| 1		| Y		| Y		| \copybrief VFS_node::report(VFS_request\*)
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::requestLock()			| 1		| Y		| Y		| \copybrief VFS_node::requestLock(VFS_request\*)
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::releaseLock()			| 1		| Y		| Y		| \copybrief VFS_node::releaseLock(VFS_request\*)
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;if **NOT** VFS_request::_isCallback	| 1		| -na-		| -na-		| will be deleted when the callback completes
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::issueResponse()  | 1->0	| N		| N		| finally we get back to the receiver
VFS_node::receiveResponse()									| 0		| Y		| Y		| the request has completed here, but a bit of cleanup is left
&nbsp;&nbsp;&nbsp;VFS_request::execute()							| 0		| Y		| maybe		| a VFS_request subclass can do something now with the satisfied request
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;if the request has a callback, VFS_request::copyCallback()	| 0		| Y		| N		| copy the payload.  This is the whole point of a request and in the case of a callback we want to transmit this.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;VFS_node::issueResponse()					| 0		| Y		| Y		| to the callback receiver
&nbsp;&nbsp;&nbsp;delete the VFS_request							| 0		| -na-		| Y		| now all action is satisfied

