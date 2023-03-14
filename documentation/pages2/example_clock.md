
\page example_clock Animated Clock

The Animated Clock plugin creates a custom pane in javascript that uses SVG animation to show the
current system time.  Because it wants to be displayed in an interface, it will need to use
the \ref client_javascript "Javascript Client" and VFS_stdlib to load into a browser.

Additional files will be included as resources and served using the VFS_creator::code()
mechanism.

The \ref client_javascript "Javascript Client" will need a user to authenticate with, so we will
create a basic one that requires no password and has a minimal interface layout.

For this, we will need to:

 - \ref example_clock_plugin "Define the plugin bundle and .pro file"
 - \ref example_clock_node "Define the clock node"
 - \ref example_clock_pane "Provide a javascript pane() subclass"
 - \ref example_clock_stylesheet "Include a stylesheet"
 - \ref example_clock_layout "Provide a layout template for the pane"

To create a \ref example_clock_freestanding "free standing example" rather than just a compiled plugin, we will need to:

 - Include the node in an xml config file
 - Create a user
 - Create an index.html file with the connection settings

\section example_clock_plugin The Plugin Bundle

The following source files will be needed, arranged in this structure:

 - \ref example_clock_pro "clock.pro"
 - \ref example_clock_h "src/clock.h"
 - \ref example_clock_cpp "src/clock.cpp"
 - \ref example_clock_qrc "src/clock.qrc"
 - \ref example_clockPlugin_h "src/clockPlugin.h"
 - \ref example_clockPlugin_cpp "src/clockPlugin.cpp"
 - \ref example_clock_html_js_clockPane_js "src/html/js/clockPane.js"
 - \ref example_clock_html_style_clock_css "src/html/style/clock.css"
 - \ref example_clock_templates_clockLayout_json "src/templates/clockLayout.json"

The \ref example_clock_pro "clock.pro" file includes the source files and the \ref example_clock_qrc "clock.qrc" resource
file, which bundles everything we will need for this plugin.  It follows the same pattern laid out
in \ref example_skeleton.

The \ref example_clockPlugin_h "clockPlugin.h" and \ref example_clockPlugin_cpp "clockPlugin.cpp" files create the plugin, which in turn creates
the \ref example_clock_cpp "clock" node and allows it to serve code.

\attention The complete clock plugin, including the brute force search algorithm, can be found in
the 'server/src/plugins/clock/src/utils' folder.  If you intend to build it, you should copy it to a work folder.
Additional files will be needed to run the example, which are explained in the \ref example_clock_freestanding "Freestanding Example" section.

\section example_clock_node The Clock Node

The \ref example_clock_h "clock.h" and \ref example_clock_cpp "clock.cpp" files define the VFS_node.  It
provides an icon in case it appears in an application menu.

The read() function sets `r->_success=true`, and sets `r->_data` to the current time, which is unused.  We need
to subclass VFS_node::read() to suppress the default warning message and to provide some non-null data to prevent the
pane manager from closing the pane immediately upon opening it.

The metadata() function defines `r->_metadata["type"]="clock:clockPane"`, which means that when the
node is read(), it will report itself as a "clock:clockPane", and therefore load the corresponding
pane type from this plugin.  The base implementation of VFS_node::metadata() will provide the VFS_node::icon()
and set `r->_success=true`.  The node also defines an "action", which tells the javascript listing() pane
to "openLayout" with the provided \ref example_clock_templates_clockLayout_json "clockLayout.json" template when the
button is clicked.  The resulting pane is placed in the "nearest" tabs() pane that can receive new tabs.

The code() function is called by clock::code().  The code could be served directly from the plugin node
itself instead of the VFS_node subclass, but it can be helpful to route code references directly to the VFS_node
that use it rather than the plugin bundler itself.  There are cases where a plugin may only bundle code and data without
serving any nodes.

Note that the type name is "clock:clockPane", not "clock:clockPane.js", even though it is returning javascript.
This is an important detail.  If the library ends with .js, it will be minified before transfer in
VFS_creator::code().  This will make the data much smaller, but also obfusticate it, making debugging much
more difficult or impossible. Because we have not saved the type into the layout file, it is trivial to
change the type in %code() and %metadata() later to include the .js for production use, where we may
not want a user to be able to easily see the source code.

\sa VFS_creator::code()

\section example_clock_layout The Clock Layout

The \ref example_clock_templates_clockLayout_json "clocklayout.json" template file gives the new tab a name and references
the path of the node in the VFS.

The class "clock" is applied to the container \<div\> in the paneManager(), which will give the stylesheet
a namespace to work with.  Note that this is an optional design choice, as the clockPane can create its
own wrapper \<div\>s to apply css to.

\sa pane::createHTML()

Note that "type" could be set to "clock:clockPane" here, which would bypass the need to set it in
metadata(), however we would need to set "keepType:true" in the layout file, or any future layout change
would cause it to revert back to type "pane".  It it always better that a node positively identify its
type, rather than force a type in the layout.  However, many times we want to view the same data through
different pane types in more complicated applications, which would require the layout to override the default.

\sa paneManager()

\section example_clock_stylesheet The Clock Stylesheet

The \ref example_clock_html_style_clock_css "clock.css" style sheet references the class "clock" found in the template.
You will want to write your css using a namespace (in this case "clock") so that future name collisions
don't happen.

Panes in the paneManager always provide an absolute positioning context, so any pane that wants to fill the
given space can create a child \<div\> and use:

\code{.css}
{
	position: absolute;
	top: 0px;
	left: 0px;
	right: 0px;
	bottom: 0px;
}
\endcode

or use

\code{.css}
{
	position: absolute;
	top: 0px;
	left: 0px;
	width: 100%;
	height: 100%;
}
\endcode

or any other css absolute positioning method.  This is especially useful if a pane wants to be subdivided.

\sa \ref themes_javascript

\section example_clock_pane The Clock Pane

The \ref example_clock_html_js_clockPane_js "clockPane.js" javascript file will be dynamically loaded when the
"clock:clockPane" type is returned from metadata().

The \ref javascript "Javascript Libraries" use <a href='http://requirejs.org'>requirejs</a> as an <a href='https://en.wikipedia.org/wiki/Asynchronous_module_definition'>AMD</a>
loader.  The pane will require other libraries to function, and those are included as a array of requirements
to the define() command.  These other libraries will be loaded asynchronously, and may even depend on each
other.  The \ref example_clock_html_style_clock_css "clock.css" is also loaded this way.  Notice that AMD
modules will, for the most part, return something as the value of the module.  The second argument to define()
is a callback function that receives the value of each module.  The exceptions for this are non-AMD things
like .css files.  Therefore, we place the css file last, and don't include a variable to record the value from
the loader.  The loader will detect the css extension and include it in a style block in the head of the html
page.

Once all the dependencies have been satisfied, the callback function is executed.  It needs to return the value
of this module, which in this case will be the clockPane prototype used by the paneManager to instantiate
the pane() subclass object, using the layout.

clockPane::createHTML() will then be called to create the gui elements.

clockPane::applySubscription() will then be called with any node data or metadata returned by
calling the subclassed VFS_node::subscribe() method in the plugin's node.  By default, applySubscription() will also call applyDiff().  In this example case this is not very interesting because the node has no metadata, data, or settings.

Now we have created the clockPane and applied any data from the layout, metadata, and node data retrieved from the VFS.  The constructor has called startTimer(), which will begin the animation.  Notice that we don't necessarily know that createHTML() has been called before the first timer event has fired, due to asynchronous network lag and potential load on the server.  Therefore, the update function will only reference variables that are known to exist (nameley members of the _digits object).  This requirement could be avoided by uncommenting the check at the head of the update function, however it is left commented to prove that the function is safe in the asynchronous loading context.

Many times you will want to include a pane::resize() function.  In this case it just re-centers the clock widget using css.

The animation math here is left as an exercise for the reader, however the overarching concept is that a digit path is updated for each animation frame.  The shape of that path is based on the vertical offset of the digit, which wants to wrap around the upper and lower pullies but also around the corners of the LCD digit.  Then, a stroke-dashoffset css paramater is used to rotate the "string" to the correct place for the current digit.  The rotations and offsets have been pre-calculated using a brute force algorithm.  The brute force source code is in the utils folder of the clock plugin included with the server source code.  It takes about an hour to run on
a mac laptop... there may be a more efficient solution, and if you're up for it, take a look!

\sa paneManager()
\sa paneLoader()
\sa pane()

\section example_clock_freestanding Running the Clock as a Free Standing Example

\attention You will need a local webserver installed to run this example.  This can be apache, nginx, %node.js, or any other http server you like.  It will need to be configured to support softlinked paths.

First, create a directory to work in and set some environment variables, then compile the plugin.  For this example we will use `~/clockExample`, and you will need to provide the actual path for VFS_DIR and VFS_PLUGINS_DIR.

\code
cd ~
mkdir clockExample
cd clockExample
mkdir libs
export VFS_DIR=[path/to/vfs_server/server]
export VFS_PLUGINS_DIR=$PWD/libs
cp -rv $VFS_DIR/src/plugins/clock ./clock
cd clock
qmake clock.pro && make -j
cd ../
\endcode

You should now be able to confirm that the clock dynamic library has been built in the libs folder.  Depending on operating
system, this will be called libclock.so or libclock.dylib.

# Create an xml config file

We will include the clock as an application, which means it will appear as a button in a user's interface, and he or she
can click on it to open the clock in a new tab.  The tab can be closed and reopened.

Create the clock.xml file in your work directory:

\code{.xml}
<vfs logLevel='9'>
    <env>
        <portstart type='int' default='3560' />
    </env>

    <hd name='users' path='users/' />

    <node name='services'>
        <nopasswd      name='nopasswd'     nopasswordfile='nopasswd' />
        <sessions      name='sessions' />
        <remotoserver  name='remotoserver' port='@portstart@+0' auth='/services/nopasswd' sessions='services/sessions' />
        <httpd_browser name='browser'      port='@portstart@+1' path='/' />
    </node>

    <node name='applications'>
        <clock name='clock' />
    </node>
</vfs>
\endcode

Notice that a __portstart__ variable is set, and ports are counted based on this starting point.  Any available port can be
chosen.  You don't have to use the \@-syntax environment variable; it could just be the port number.  Refer to \ref configfiles
for more information.  We're using the `@portstart@` variable here so the VFS_httpd_browser node can increment from it.

# Create the user

The VFS_nopasswd authentication node needs the username listed in the nopasswd file.

Our user will need an entry:

\code
echo "clockuser" >> nopasswd
\endcode

The user will need a layout file and a preferences file:

\code
mkdir -p users/clockuser
cd users/clockuser
echo "{}" > preferences.rfm
mkdir layouts
touch layouts/current.rui
cd ../../
\endcode

Edit the empty users/clockuser/layouts/current.rui file, and paste this into it:

\code{.js}
{
    "type": "splitter",
    "name": "Application Main",
    "sizes": [ "64px", null ],
    "stacking": "horizontal",
    "fixedSize": true,
    "children": [
        {
            "name": "Applications",
            "path": "applications",
            "type": "pane"
        },
        {
            "children": [],
            "name": "Tabswitcher",
            "selected": 0,
            "type": "tabswitcher"
        }
    ]
}
\endcode

For this example, the preferences file will be an empty json object and the layout will have our applications
menu and a tabswitcher to place the clock.  When the user clicks on the clock icon, it will open in a new tab.

# Create an index.html file

If you haven't, you will need to clone \ref client_javascript repository.  We will need to create a client directory and an
index.html file that will be served by a webserver and will tell the bootstrap.js and vfsClient what port and path to connect
to.  You will need to provide the actual path to the client repository.

\code
export VFS_CLIENT_DIR=[path/to/vfs_client_javascript]
mkdir client
cd client
ln -s $VFS_CLIENT_DIR/images images
ln -s $VFS_CLIENT_DIR/js js
ln -s $VFS_CLIENT_DIR/style style
touch index.html
cd ..
\endcode

Edit the client/index.html file and paste the following:

\code{xml}
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:svg="http://www.w3.org/2000/svg">
	<head>
		<link rel="stylesheet" href="style/style.css" />

		<script type='text/javascript'>
			window.VFS_config = {
                                                "ws": {
                                                    "applicationName": "Remoto Clock",
                                                    "host": "localhost",
                                                    "path": "",
                                                    "port": "3560",
                                                    "version": "v0.8"
                                                }
                                            };
		</script>

		<script data-main="js/main" src="js/require/require.js"></script>
	</head>
	<body>
	</body>
</html>
\endcode

If you are running this in a local development environment like a laptop or workstation, the host will be 'localhost', the
port will be the one you chose, and the path will be an empty string.  More advanced configurations are discussed in \ref nginx.

# Run the VFS with this configuration

You can now point the VFS executable to the files that have been created:

\code
$VFS_DIR/VFS ./clock.xml -plugins $VFS_PLUGINS_DIR
\endcode

You should see the VFS build itself and log what ports have been opened.

Open it in a browser:

\code
http://localhost/[path/to/index.html]
\endcode

You will need to login with the user we created: `clockuser`.  The password field will be left empty.

As a shortcut for direct linking:

\code
http://localhost/[path/to/index.html]?username=clockuser
\endcode

Clicking on the clock icon will open it in a new tab.  By default only one tab per application can be opened.  If you close
the tab, it can be reopened.  Changes to the layout are recorded.  Reloading the page will retain the current state for the
user.  If multiple browser windows with the same user are simultaneously logged in, they will synchronize.

Also notice that we've opened a port for the VFS_httpd_browser, which provides some diagnostic information about the running VFS.
This tool is for development use and can be very helpful when debugging connections and viewing the current state of things.  It can be
accessed here:

\code
http://localhost:3561
\endcode

You may need to change the port to the one you're using, as it is incremented from the `@portstart@` variable.  The VFS_httpd_browser
shows the VFS_node::report() information of each node in the VFS.  Click on the "expand reports" button to open the tree.  When a user
(clockuser) is logged in, you will see it as a child of the VFS_remotoserver node.  When we start to use multithreading, the thread
addresses can be useful to check. Subscriptions are also listed here.  Notice that the clockuser is subscribed to both the `clock`
node and the `applications` directory. If a new application is added by another user, it will appear here in realtime.  Once the 'clock'
button has been clicked, check the contents of the layout file to see what has changed.

\section example_clock_summary Summary

We've visited a few things here:

 - how to define a plugin that hosts a single node, but could be many.
 - how to define a node that views a custom javascript pane.
 - how to define a fairly complex javascript pane for the node that uses SVG animation.
 - how to include the node in a config file, create a user, and run the VFS executable with this config file.
 - how to view some diagnostic information using the VFS_httpd_browser node.

\ref example_screencontroller "The Screen Controller Example" explains how to connect to real world devices, synchronize
interfaces for multiple users, and more about subclasses.

------

\section example_clock_source Source Files

\anchor example_clock_pro

###clock.pro

\include clock/clock.pro

\anchor example_clockPlugin_h

###src/clockPlugin.h

\include clock/src/clockPlugin.h

\anchor example_clockPlugin_cpp

###src/clockPlugin.cpp

\include clock/src/clockPlugin.cpp

\anchor example_clock_qrc

###src/clock.qrc

\include clock/src/clock.qrc

\anchor example_clock_h

###src/clock.h

\include clock/src/clock.h

\anchor example_clock_cpp

###src/clock.cpp

\include clock/src/clock.cpp

\anchor example_clock_templates_clockLayout_json

###src/templates/clockLayout.json

\include clock/src/templates/clockLayout.json

\anchor example_clock_html_style_clock_css

###src/html/style/clock.css

\include clock/src/html/style/clock.css

\anchor example_clock_html_js_clockPane_js

###src/html/js/clockPane.js

\include clock/src/html/js/clockPane.js

