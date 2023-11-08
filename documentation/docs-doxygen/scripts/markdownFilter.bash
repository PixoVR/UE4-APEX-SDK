#!/bin/bash

INFILE=$1

FILENAME=`basename "$INFILE" .md | perl -pe "s/ /_/g"`
FILEDIR=`dirname "$INFILE"`
#FILEDIR="$( cd "$( dirname "$INFILE" )" && pwd )"

#DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

LINES=$((0+`cat "$INFILE" | wc -l`))
#echo lines: $LINES

# the case for a softlink that was incorrectly resolved somewhere in the git hydration process,
# and was committed back as a single line pointing to its resolved link.
if (("$LINES" < "2"));
then
	INFILE=$FILEDIR/`cat "$1"`
	#echo $INFILE;
fi

PAGEDEF=`head -n 3 "$INFILE" | grep -E "\\\\\(mainpage|page)"`
PAGETITLE=`head -n 5 "$INFILE" | grep -E "^#\\s" | head -n 1 | perl -pe "s/^#\\s(.*)/\1/"`

[[ -z $PAGETITLE ]] && PAGETITLE=$FILENAME

[[ -z "$PAGEDEF" ]] && echo -e "\\page $FILENAME $PAGETITLE\n"
#[[ -z "$PAGEDEF" ]] && echo -e "\\defgroup $FILENAME $FILENAME\n@{\n\\ingroup pages\n"

# add an author?  This is tricky because it wants a context, and different languages are different.
#echo "/** \author " `cd ../../; git log --format='%an' -n 1 $INFILE` " */"
#echo

[[ -z "$PAGEDEF" ]] && awk '!/# / || f++' "$INFILE" || cat "$INFILE"

#[[ -z "$PAGEDEF" ]] && echo -e "\n@}\n"

