#!/bin/bash

CONFIG_MD=$1

if [[ -z "$CONFIG_MD" ]]
then
	echo $@
	echo "CONFIG_MD required."
	exit 1
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/../

source ../env.sh

MD=`echo $CONFIG_MD | perl -pe "s/\//\\\\\\\\\//g"`

find `echo $DOXYGEN_INPUT | perl -pe "s/\"//g"` \
	-false \
	-o -name "*.uplugin" \
	-o -name "*.uproject" \
	-o -name "*.ini" \
	| \
	perl -pe "s/.*\/([^\/]*)/\1/" \
	| \
	perl -pe "s/[^\w]/_/s" \
	| \
	perl -pe "s/(.*)/.\/scripts\/append_configs_doc.bash $MD \1/" \
	| \
	bash -v


