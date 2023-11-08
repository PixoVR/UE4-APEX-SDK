#!/bin/bash

export PROJECT_NAME='Name of project'					# should be short enough to fit on the documentation page
export MENU_WEIGHT=100							# a sort index which will be applied before alphabetization
export PROJECT_VERSION=`cd ../; git tag | tail -n 1`			# the last tagged version
export PROJECT_BRIEF='Documentation for the Xyzproject C++ Library'	# a brief description of the SDK or module
export PROJECT_STATUS='active'						# project status, for publishing.  Recognized choices are 'active' (default if not present), 'inactive' (invisible and won't appear in the documentation menu), 'deprecated' (visible in menu, but marked deprecated)
export PROJECT_LOGO='docs-doxygen/doxygen-custom/defaultIcon.png'	# an icon for the submodule, or 'docs-doxygen/doxygen-custom/defaultIcon.png'.  Path is relative to the folder containing `build.sh`.  Should be 128x128 transparent png, probably round shaped.
export PROJECT_REPO='https://github.com/PixoVR/somerepo'		# the project repo url, for cloning
export PROJECT_URL='/SomeSDK-Target'					# subfolder used for documentation: 'https://docs.pixovr.com/SomeSDK-Target', like 'ApexSDK-Unreal'
export DEV_PROJECT_URL='../../../../Unreal/SomeSDK-Target/documentation/html'	# a url for local development, which is used when `docs-root` is publishing on a local system via `DEV=true ./build.sh`.  This will usually just be to replace `SomeSDK-Target` with the repo name.
export PROJECT_MAIN_PAGE='../pages/mainpage.md'				# the main home markdown page for the documentation

export DOXYGEN_FILTER='scripts/unreal_filter.py'			# a script filter for interpreting the code (adds stuff like decorators or macros)
export DOXYGEN_INPUT='  "../../Source"
			"../../Folder With Spaces"'			# a list of input folders for documenting, which is a whitespace-separated list of (optionally) quoted paths.  The Doxyfile will already include "../pages" for you.  Paths are relative to the `docs-doxygen` folder.
export DOXYGEN_IMAGES=''						# a list of root folders to look for images
export DOXYGEN_EXCLUDE=''						# a list of paths to exclude when building.
export DOXYGEN_EXCLUDE_PATTERNS=''					# a list of patterns to exclude when building.
export DOXYGEN_STRIP_FROM_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../" && pwd)"	# the full absolute path to the root folder to be removed during publish.  This is kind of cosmetic but helps reduce confusion when finding libraries.
export DOXYGEN_IGNORE_PREFIX=''						# a class/method/variable prefix to be ignored when alphabetizing.  For instance if everything is pApexSDK, mMatrix, etc, the ignore prefix may want to be "p m" where the list is a whitespace-separated list of prefixes.  The ignore order matters, where longer entries should be first.
export DOXYGEN_CSS=''							# a list of css files to add.  Note that these are relative to docs-doxygen, and that "../style.css" is already added.  This will rarely need to be changed.

# user variables.  Useful when writing pages that refer to other urls, but don't want to hardcode them in.  For instance, use `$(APEX_SERVER_URL)` in a markdown page to resolve the variable reference.
#export APEX_SERVER_URL='https://apex.pixovr.com'

# these are only useful when using the Dockerfile-unreal* stuff
#export UNREAL_UFS_PATHS='/Game'					# UFS input paths for publishing blueprints, materials, etc.  Probably '/Game' for .uproject, and a plugin-specific UFS path(s) for .uplugin(s)
# only used for Dockerfile-unreal[45]-project
#export UNREAL_PROJECT='/project/someproject.uproject			# UFS project to publish
