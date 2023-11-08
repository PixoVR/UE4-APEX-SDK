#!/bin/bash

CONFIG_MD=$1
ITEM=$2

if [[ -z "$CONFIG_MD" ]] || [[ -z "$ITEM" ]]
then
	echo $@
	echo "CONFIG_MD and ITEM required."
	exit 1
fi

ITEM=generated_`echo -n $ITEM | perl -pe "s/[^\w]/_/g"`

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/../../

#if [ -s $CONFIG_MD ]
if ! (grep "\\\\page config_files" $CONFIG_MD &> /dev/null)
then
	echo "/**" >> $CONFIG_MD
	echo "	\\page config_files Configuration Files" >> $CONFIG_MD
	#echo "	\\ingroup modules" >> $CONFIG_MD
	echo "	\\brief Configuration files found in the **\$(PROJECT_NAME)** repository" >> $CONFIG_MD
	echo >> $CONFIG_MD
	echo "	- \\subpage $ITEM" >> $CONFIG_MD
	echo "*/" >> $CONFIG_MD
else
	# replace comment end with new item, plus comment end
	sed -i.tmp "s/\*\//	- \\\\subpage $ITEM\n\*\//" $CONFIG_MD
	rm ${CONFIG_MD}.tmp
fi

