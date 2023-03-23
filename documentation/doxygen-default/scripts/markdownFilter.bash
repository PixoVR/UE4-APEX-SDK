#!/bin/bash

FILENAME=`basename "$1" .md | perl -pe "s/ /_/g"`

PAGEDEF=`head -n 3 "$1" | grep -E "\\\\\(mainpage|page)"`
PAGETITLE=`head -n 5 "$1" | grep -E "^#\\s" | head -n 1 | perl -pe "s/^#\\s(.*)/\1/"`

[[ -z "$PAGEDEF" ]] && echo -e "\\page $FILENAME $PAGETITLE\n"
#[[ -z "$PAGEDEF" ]] && echo -e "\\defgroup $FILENAME $FILENAME\n@{\n\\ingroup pages\n"

[[ -z "$PAGEDEF" ]] && awk '!/# / || f++' "$1" || cat "$1"

#[[ -z "$PAGEDEF" ]] && echo -e "\n@}\n"

