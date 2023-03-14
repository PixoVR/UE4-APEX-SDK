
\page installation Installation

The common installation will include a VFS server and an http(s) server.  The few pages served by http will
send enough information to a browser to initiate a websocket connection to the VFS and get things going.

The VFS server component will want to run somewhere that is accessible to the address mentioned in the
index.html file.  An http server (nginx, apache) will need to serve those root elements, and the rest is
encapsulated by the VFS server.

If you are using a C# Client (Unity) or a Python Client, there is no need for a web server.

A basic installation using http/javascript:

 - Pull down the VFS_server source, and use the \ref server instructions to build it.
 - Pull down the VFS_client_javascript source (\ref client_javascript), .
 - An easy configuration is to create an additional folder in your working directory, and softlink everything
 except index.html in that directory to the client folder.  Then, copy index.html to that folder and make changes
 as needed.  This requires your webserver to follow simlinks, but allows you to change index.html as you like
 without making changes to the javascript repository.

\todo DOCME

These subpages explain the installation and usage process:

 - \subpage server
 - \subpage clients
 - \subpage documentation
 - \subpage nginx
 - \subpage docker
 - \subpage mongocxx
 - \subpage plugins
