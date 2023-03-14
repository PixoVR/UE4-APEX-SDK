
\page docker Docker Use and Configuration

# Docker

You will want to use some flavor of the `containerize.sh` script.  This will pull variables from a .env file, set a name for the container,
and create a docker image on a local machine.  Optionally, "-p" will push the image to the repository.

When `containerize.sh` is successful, you can move to the `do.sh` script, which will instantiate docker instances.

You will want as few variables as possible in `docker-compose.yml`.  The VFS server will always be included in some other package,
and doesn't do anything useful by itself.  `do.sh` will not appear in the vfs_server source.  Good examples are made for mportal, mfinder, and
mrendermanager.

Read the \subpage dockerfile "Docker Files" reference page.

Read the \subpage dockerbuild "Building the Containers" reference page.

use vfs_qtbase to build.  Be sure you have at least 50-60GB of free space because the Qt sourcecode expands to be huge.
