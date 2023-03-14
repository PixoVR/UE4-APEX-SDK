
\page configfiles XML Config File(s)

\todo config files... much more here... add documentation for logLevel, monitor, version

A %VFS server wants to read an XML config file upon launch to configure its node tree.  Examples are provided in the server/configs folder.  Environment variables can be set before running the executable, or mentioned as name=value pairs.

Environment variables can be referenced in the config file using `@variable@` syntax.  Values which are numeric, like port numbers, can use basic math with ECMA scripting.  This allows a start port value to be defined and other ports can increment from there.

Several variables are defined for convenience and context:

 - `@cwd@` - This will be the directory of the currently executing config file.  This is useful for providing a mechnism like relative paths, so that absolute paths are not so needed.
 - `@configfile@` - the currently executing config file.  This is useful because config files can include other config files, so the context is needed.
 - `@vfscwd@` - the CWD in the %VFS.  Nodes are created sequentially and are nested.  Sometimes a node wants to know its position in the %VFS.
 - `@approot@` - the directory containing the %VFS executable

XML tags available in a config file:

 - `<env>` - a space to define variables for use in other nodes.  This works like a series of shells.  Config files included will inherit the current context but when the include is complete, that shell context will be exited.
 - `<include path=[configpath] [name=value ...]/>` - to include other config files.  Provide an absolute path or one relative to the current config file.  Optionally, name=value pairs of environent variables can be sent to the include context.
 - Any nodes provided by VFS_stdlib, or any nodes provided by loaded plugins.

Because it's XML, comments can be embedded as text.  Only XML tags are read by the parser.  All nodes require a name.  Any node can be placed on a different processing thread by adding thread='1'.  Nodes will inherit the thread of their parent unless they explicitly opt to create a new one.  In unique cases, one may want a VFS_threadpool for child nodes, which can be achieved by setting `thread='X'` on a tag, where X is the number of threads to create for the pool.

Any node and its children can be deactivated by setting active='false'.


\code{.xml}

<VFS version='1.0' logLevel='8'>

	<env>
		//admin email is used for CRITICAL messages
		<adminemail	type='string'	default='admin@domain.com' />
		<portstart	type='int'	default='6200' />
		<dataroot	type='string'	default='@approot@/../data/main' />

		//apache default: /etc/pki/tls/certs/localhost.crt
		<sslCert	type='string'	default='../workspace/ssl2/server.crt' />
		//apache default: /etc/pki/tls/private/localhost.key
		<sslKey		type='string'	default='../workspace/ssl2/server.key' />

		//settingsroot is for the HD node, while settings is an env for applications
		<settingsroot	type='string'	default='@dataroot@/settings' />
		<settings	type='string'	default='settings' />
	</env>

	<hd name='users' path='@dataroot@/users/' thread='1' />
	<hd name='settings' path='@settingsroot@' create='1' thread='1' />

        <aclnode name='services'>
                //cron should always be in its own thread
                <cron name='cron' thread='1' />

                //auth stuff
                <node name='auth'>
                        //token data should be in a place that is not shared by another vfs path (make sure there's no overlap with another VFS_HD, for instance)
                        <hd             name='tokens' path='@cwd@/../../data/main/sessions' thread='1' />

                        //auth types
                        <nopasswd       name='nopasswdauth' nopasswordfile='@cwd@/../../data/main/settings/@vfscwd@/nopasswd' thread='1' />
                        <passwd         name='passwdauth' passwordfile='@cwd@/../../data/main/settings/@vfscwd@/passwd' thread='1' />
                        <pam            name='pamauth' service='@pam_service@' group='staff' thread='1' />
                </node>

                //ACL entries
                <acl            name='ACL' path='settings/@vfscwd@/master.acl' default='true' superadmin='@superadmin@' />
          
                //session manager for connected users... sessions will timeout after 30 days
                //token data and userdata are VFS paths
                <sessions name='sessions' userdata='home' tokendata='services/auth/tokens' ttl='60*24*30' developerGroup='admin' thread='1' />

                //websocket server for all connections
                <remotoserver   name='remotoserver'     port='@websocketPort@+0'        auth='services/auth/pamauth' sessions='services/sessions' thread='1' ssl='@wsUseSSL@' sslCert='@wsSslCertPath@'>

                //httpd server for checking current state of things
                <httpd_browser  name='browser'          port='@websocketPort@+2'        path='/' thread='1' ssl='@wsUseSSL@' sslCert='@wsSslCertPath@' sslKey='@wsSslKeyPath@'/>
        </aclnode>

	<node name='applications'>
		<clock name='clock'/>
		<codeEditor name='IDE' src='@approot@/../' thread='1'/>
		<include path='modules/renderermanager.xml' portstart='@portstart@' dataroot='@dataroot@/renderer' />
		<include path='modules/nuke.xml' gui='1' projectsroot='@dataroot@/nuke' commandportstart='@portstart@+11' />
	</node>

</VFS>

\endcode

