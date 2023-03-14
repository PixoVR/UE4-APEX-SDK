
\page example_logview Log Viewer

The log view plugin will open a textStream pane to view the contents of the VFS_stream nodes associated with the VFS logging mechanism.

This is very similar to \ref example_skeleton, but adds a few important pieces.

The adminPlugin class provides the logView node (which is named "logview" in a config file).  The logView node effectively proxies VFS_node::read() and VFS_node::metadata() requests to VFS log paths.

For convenience, this can be added to the root `applications` node so it will appear as an application button in the the user's menu:

\code{.xml}
<VFS>
	<node name='applications'>
		<logview name='Log Viewer' thread='1'/>
	</node>
</VFS>
\endcode

Note that the logView node can be placed on another thread if desired, but importantly the VFS::_logger node (created as a private member of the VFS) is also placed on another thread.  Traffic on the main VFS thread can be heavy, so we want to distribute the work as much as possible.

\section example_logview_breakdown Breaking down the logView code

For reference: logView.cpp

The logView node overrides:

 - VFS_node::find()
 - VFS_node::icon()
 - VFS_node::read()
 - VFS_node::metadata()

The logView::find() method will always return `this`, because the logView needs to proxy all requests to an actual VFS path, which will be `logs` (on the root of the VFS).  We don't want a search using VFS_node::find() to descend to children of this node.

The logView::icon() method will return the "stream" icon from VFS_icons.  This is the button we see in the interface.

The logView::read() method will forward all read requests with an empty path to the root VFS `logs` path.  Any request with a non-empty path will call VFS_node::read(), which will result in an error.

The logView::metadata() method will check to see if a request path is empty (which means that this node is being directly queried), or if a subpath component exists, in which case it will tweak the returned icon to have a color.  In either case, it will return a layout template for showing the contents of a log path.
