#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/doxygen

source ../env.sh

[ -f "mdfiles.txt" ] && rm mdfiles.txt
[ -f "rmfiles.txt" ] && rm rmfiles.txt

rm -rf ../html
cp -rfv ../../*md ../pages/ | perl -pe "s/.* -> (.*)/\1/" >> rmfiles.txt

doxygen doxyfile

cp -rv images ../html/images

exit

if [ -f "mdfiles.txt" ]; then

	cat mdfiles.txt | while read line
	do
		#grep "\\page" $line | perl -pe "s/.*\\\page\s*(^\s*).*/\1/"
		#echo $line
		TAG=`grep "\\\\\page" $line | cut -f 2 -d " "`
		#echo TAG: $TAG
		[[ ! -z "$TAG" ]] && \
			echo CLEANING: $TAG && \
			sed -i.bak "/$TAG/d" ../html/*.js && \
			sed -i.bak "/$TAG/d" ../html/search/*.js
	done
	rm mdfiles.txt
fi

if [ -f "rmfiles.txt" ]; then

	cat rmfiles.txt | while read line 
	do
		rm $line
	done

	rm rmfiles.txt
fi

cd ../

