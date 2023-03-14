
\page philosophy Philosophy

\section whyVFS Why VFS?

Companies like Google and Amazon need vastly more computing power than can be maintained
by a single big iron system.  Data centers were created, and computing has become a
multithreaded, multi-machine equation.  Software needs to respond to this and provide a
layer of interaction that appears unified and agnostic to the underlying mechanisms.  We
see the results of this constantly but don't think about it much.  What has happened in
the last few years is the refinement of various technologies and processes that are focused
on scalability, consistency, repeatability, and agnosticism.  These technologies created
and enabled “the cloud” and continually feed back into themselves.

Much of the impetus for %VFS has been a continual need for abstraction.  Programmers have
been building abstractions, languages, languages that output other languages, operation
systems that network, networks that network, virtualization, containerization, and now
Amazon is offering cloud scale algorithm processing.

The goal and result of all this work has been to separate computing from computers.  We no
longer try to think about running processes on hosts, we have a layer that communicates with
hosts and networking and administration layers which enable us to apply a set of resource
constraints to a process and allow a cluster to manage where it will run.  Evolving
alongside this, for better or worse, is the “internet of things”, which are merely internet
enabled objects like shoes, clothes, umbrellas, fridges, coffee makers, the list goes on.
The user interface, although more code than any other part of the design, is an artifact of
the desire to talk to a system.

VFS is an abstraction layer that can connect this increasingly diverse set of resources and
allow users to exploit all these disparate but powerful technologies.  One of the greatest
strengths of %VFS is the ability to provide an evolving but consistent platform for user tools
while implementation details and changes to the underlying ecosystem are hidden by the driver
level.  It runs well with Docker and VM's, and is based on the Qt Framework, which can be
compiled on many platforms.

VFS connects endpoints together with intelligence in between.  Commonly this will be exhibited
as subscriptions, where a button on an interface talks to a file, a state is changed, and
subscribers are notified.  But it could also be a device reporting its state at 60hz and other
mechanisms read and respond to it.  The VFS uses paths to name resources.  Users, servers,
disks, joysticks, VR headsets, or theortecially anything can be abstracted as a VFS_node and
used as an endpoint.

\section isntThisTheInternet Isn't this just what The Internet does?

In short, Yes.  Both The Internet and VFS are pathed systems of resources that can be called
upon to provide responses to requests, and a variety of devices on either end of the communcation
can participate.

Of course, this depends on how you define The Internet; it can be viewed as a large set of
html webpages that encompass all digital human content.  On a deeper look, it's a set of named
resources connected via DNS mechanisms that may exhibit themselves as webpages.  The Internet Of
Things is real, it just hasn't matured yet.

The Internet is heavily based on discreet http requests for pages of content, and http requests
aren't particularly well designed for realtime control of things, and don't necessarily expand to
their full potential outside of the context of a web browser.  Websockets allow a browser to
expand this, but they also ask for a paradigm shift in the concept of content and usability.

We're seeing a lot of Internet activity happening outside of the web browser, but they tend to be
very silo-ed services that don't represent a unified platform for heavy interoperation, and they
don't necessarily access the full potential of a local (or remote) machine's resources, especially
in the context of OpenGL graphics, VR, cameras, microphones, controllers, or any other thing a
developer may choose to talk to.  The closest thing we have are gaming systems, which tend to
be walled gardens of cool stuff, but not particularly unified in how they speak to each other, and
not designed to be global citizens in The Internet Of Things.

\section whyNotUnityUnreal Why not Unity or Unreal or Server X?

These engines are great, and the %VFS doesn't intend to duplicate what they do well.  There are
overlapping functionalities that the developer needs to choose between.  %VFS can act as
a linkage between parts or a more comprehensive engine with smarts inside of it.  A
compelling reason to use Unity, for instance, is that lots of user-level math in 3D space has
been completed and tested on many systems with different interfaces.  But you may just want an
html page that talks to a server for controlling videos.  %VFS can be installed on a
Raspberry Pi to report the temperature of your greenhouse to another system, or to control the
ailerons of a drone, or the play/pause/chapter state of a video.

The VFS connects paths together so they don't need to know much about each other to communicate,
as long as they communicate the basic \ref commands in the protocol.

\section vfsgoals Goals for the VFS

Admittedly it takes a bit of apparatus to add a thing to the VFS.  The \ref examples page shows what
is required.  There are a series of rules to observe and multithreading is no joke.  The VFS wants to
connect parts together, and conforming the parts can be a philosophical challenge.  The system
originated with read, write, submit, and subscribe commands, but other commands have crept into the
system with metadata options.  The spirit still stands; keep things as simple as possible, and allow
the names of the functions to do their job.  Make many nodes, not a master node with a lot of "if"
statements.  Subclass when possible; group functionality together and subclass for specific cases.

Using VFS_tcp_mount, one VFS can mount itself into another VFS.  Commonly we will use one VFS as the
main interface, and sub-VFS instances will perform work on other machines, passing their interface
and code to another VFS.

__The goal of the VFS is to create an ecosystem of nodes that conform to a basic communication protocol__.

\ref installation "Installing" the VFS is a good first step, and seeing how the parts talk to each other
will give you ideas, especially after you install the examples.


