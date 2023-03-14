#!/bin/bash

# README: Run in project root to build docker images
# Optionally pass args:
#	-f 		: force tag, docker versions before v1.10.0
#	-c 		: USE cache, disable default --no-cache
#	-n <image_name> : override image name, eg. registry.themolecule.com/vfs/vfs_server
#	-t <image_tag>	: override image tag, eg. v1.1.0
#	-p 		: push to registry
#	-h		: help
#

image_name="[pixo docker registry]:5050/ApexSDK"
image_tag="prod-docs"

##### DEFAULTS #####
legacy_force_tag=""
no_cache="--no-cache"
docker_push=""

docker login [pixo docker registry]:5050

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

while getopts ":fcn:t:p" opt; do
	case $opt in
		f) legacy_force_tag="-f"
		;;
		c) no_cache=""
		;;
		n) image_name="$OPTARG"
		;;
		t) image_tag="$OPTARG"
		;;
		p) docker_push="true"
		;;
		*) printf "Valid args are:\n -f (force tag)\n -c (USE cache)\n -n $image_name (set name)\n -t $image_tag (set tag)\n -p (docker push)\n"
		exit 1
		;;
	esac
done

##### PREP #####
cd $DIR


##### BUILD #####
image_id=`docker build $no_cache -f Dockerfile ./ | tee /dev/tty | tail -n 1| awk '{print $3}'`


##### CLEANUP #####



####################

if [ -z "$image_id" ]; then
	echo "ERROR: Build failed: $image_name:$image_tag"
	exit 2
else
	echo "Build Image: $image_id"
fi

if docker tag $legacy_force_tag $image_id $image_name:$image_tag; then
	echo "Tagged: $image_name:$image_tag"
else
	echo "ERROR: Tag failed: $image_name:$image_tag"
	exit 2
fi
	
if [ "$docker_push" ]; then
	if docker push $image_name:$image_tag; then
		echo "Pushed to registry as $image_name:$image_tag"
	else
		echo "ERROR: Failed to push to registry."
		exit 1
	fi
fi

