#!/bin/bash

export PROJECT_NAME='ApexSDK - Unreal'
export PROJECT_VERSION=`git tag | tail -n 1`
export PROJECT_BRIEF='Documentation for the Unreal C++ Library'
export PROJECT_LOGO='../../Resources/Icon128.png'
export PROJECT_REPO='https://github.com/PixoVR/UE4-APEX-SDK/'
export PROJECT_URL='/ApexSDK-Unreal'
export DEV_PROJECT_URL='../../../../Unreal/UE4-APEX-SDK/documentation/html/index.html'
export PROJECT_MAIN_PAGE='../pages/README.md'

export DOXYGEN_FILTER='../scripts/unreal_filter.py'
export DOXYGEN_INPUT='"../pages"\n"../../Source/ApexSDK"'
export DOXYGEN_STRIP_FROM_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../Source/ApexSDK" && pwd)"

export APEX_SERVER_URL='https://apex.pixovr.com'
