#!/bin/bash

#echo "DOT WRAPPER: " "$@" 1>&2

TYPE=$2
SVG=$4

#echo "DOT WRAPPER: " "$TYPE" 1>&2
#echo "DOT WRAPPER: " "$SVG" 1>&2

# run this no matter what, and suppress the warning about size,
# which we know is because we're bending DOT's ability to render
# unreal blueprints because DOT wants a node's point to be in the
# center, but blueprints want a nodes position to be the upper left.
dot "$@" 2>&1 | grep -v "too small for content" | grep -v "in label" | grep -v "layers not supported"

case $TYPE in

	-Tcmapx)
		;;

	-Tsvg)

		#exit
		if [[ "$SVG" == *"inline"* ]]; then

			#echo "dot_wrapper.sh: " "$@"
			echo "Using dot_wrapper.sh to correct bounding box: '`basename $SVG`'"

			# rebuild the svg's bounding box, because the blueprints render out of bounds.
			#echo "dot_wrapper fixing bounding box: " "$SVG"
			inkscape -D "$SVG" --export-type=svg -o "${SVG}.svg" 2>/dev/null

			VIEWBOX=`grep "viewBox" ${SVG}.svg | sed -e 's/^[[:space:]]*//' | perl -pe 's/"/\\\"/g'`
			WIDTH=`grep " width=" ${SVG}.svg   | sed -e 's/^[[:space:]]*//'` # | perl -pe 's/"/\\\"/g'`
			HEIGHT=`grep " height=" ${SVG}.svg | sed -e 's/^[[:space:]]*//'` # | perl -pe 's/"/\\\"/g'`
			#echo VIEWBOX: $VIEWBOX

			# doxygen only wants ints for viewbox
			VIEWBOX=`echo -n "$VIEWBOX" | perl -pe 's/(\d+)\.?(\d*)/\1/g'`

			# add margin to the int values
			export `echo $VIEWBOX | perl -pe "s/viewBox=.+(\d+) (\d+) (\d+) (\d+).*/A=\1\nB=\2\nC=\3\nD=\4\n/"`

			F=16		
	
			#C=$(( (((C/$F)+0)*$F) ))
			C=$((C+10))
			#D=$(( (((D/$F)+1)*$F) ))
			D=$((D+15))
			VIEWBOX="viewBox=\"$A $B $C $D\""
			#echo VIEWBOX: $VIEWBOX

			# truncate to an int
			W=`echo -n "$WIDTH"  | perl -pe 's/(.*=\")(\d+)(\.?.*)/\2/'`
			H=`echo -n "$HEIGHT" | perl -pe 's/(.*=\")(\d+)(\.?.*)/\2/'`
			#W=$(( (((W/$F)+1)*$F) ))
			W=$((W+15))
			#H=$(( (((H/$F)+0)*$F) ))
			H=$((H+10))
			WIDTH=" width=\"${W}pt\""
			HEIGHT=" height=\"${H}pt\""
			#echo DIMS: $WIDTH $HEIGHT

			# actually replace in the svg file
			sed -i'.tmp' "s/viewBox=\"[0-9 .]*\"/${VIEWBOX}/" ${SVG}
			sed -i'.tmp' "s/ width=\"[0-9 .pt]*\"/ ${WIDTH}/" ${SVG}
			sed -i'.tmp' "s/ height=\"[0-9 .pt]*\"/ ${HEIGHT}/" ${SVG}

			#echo NEW VIEWBOX: $VIEWBOX
			#echo NEW WIDTH: $WIDTH $W
			#echo NEW HEIGHT: $HEIGHT $H

			# clean up
			rm "${SVG}.svg"
		fi

		;;

	*)
		;;
esac
