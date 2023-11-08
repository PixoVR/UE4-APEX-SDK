#!/bin/bash

INFILE=$1

if [[ "$INFILE" == "" ]]
then
	exit 0
fi

echo "Filtering '$INFILE'..." 1>&2

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/../../

FILENAME=`basename $INFILE`
PAGENAME=generated_`echo -n $FILENAME | perl -pe "s/[^\w]/_/g"`
TITLE="Unknown File"

# this won't work because doxygen's preparser will not see the results...
# it will parse the generated file before it has been generated.
#CONFIG_MD="pages/generated_configs.dox"
#flock -e ${CONFIG_MD}.lock $DIR/append_config_doc.bash $CONFIG_MD $PAGENAME

echo "\\page $PAGENAME $FILENAME"
#echo "Some stuff about the $FILENAME config file."
#echo $INFILE
echo
echo "\\code{.xml}"
cat $INFILE
echo "\\endcode"
