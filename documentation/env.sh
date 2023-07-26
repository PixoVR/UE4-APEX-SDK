#!/bin/bash

export PROJECT_NAME='ApexSDK - Unreal'
export MENU_WEIGHT=50
export PROJECT_VERSION=`cd ..; git tag | tail -n 1`
export PROJECT_BRIEF='Documentation for the Unreal C++ Plugin'
export PROJECT_STATUS='active'						# project status, for publishing.  Recognized choices are 'active' (default if not present), 'inactive' (invisable and won't appear in the documentation menu), 'deprecated' (visible in menu, but marked deprecated)
export PROJECT_LOGO='../Resources/Icon128.png'
export PROJECT_REPO='https://github.com/PixoVR/UE4-APEX-SDK/'
export PROJECT_DEMO='https://github.com/PixoVR/UE4-APEX-SDK-Demo/'
export PROJECT_URL='/ApexSDK-Unreal'
export DEV_PROJECT_URL='../../../../Unreal/UE4-APEX-SDK/documentation/html'
export PROJECT_MAIN_PAGE='../pages/mainpage.md'

export DOXYGEN_FILTER='scripts/unreal_filter.py'
export DOXYGEN_INPUT='"../../Source/ApexSDK"
			"../../ApexSDK.uplugin"'
export DOXYGEN_STRIP_FROM_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../Source/ApexSDK" && pwd)"
export DOXYGEN_IGNORE_PREFIX='FXAPI F U'

export APEX_SERVER_URL='https://modules.apex.pixovr.com/'
export APEX_MODULESERVER_URL='https://modules.apex.pixovr.com'
export XAPI_REPO_URL='https://github.com/adlnet/xAPI-Spec'
