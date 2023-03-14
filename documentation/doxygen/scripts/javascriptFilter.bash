#!/bin/bash

#cat $1 | perl -pe "s/Chrome/Madonna/i"

#cat $1\
#| perl -pe "s/(.*define\(.*)/\/\/\/ \\\cond\n\1\n\/\/\/ \\\endcond/"\
#| perl -pe "s/(.*\.prototype\s*=.*)/\/\/\/ \\\cond\n\1\n\/\/\/ \\\endcond/"\
#| perl -pe "s/(.*\.prototype\..*=.*function\(.*\))/\/\/\/ \\\cond\n\1\n\/\/\/ \\\endcond/"\
#| perl -pe "s/(^\s*[{}]\s*\$)/\/\/\/ \\\cond\n\1\/\/\/ \\\endcond\n/"

#| perl -pe "s/(.*\s+function.*)/\/\/\/ \\\cond\n\1\n\/\/\/ \\\endcond/"
#| perl -pe "s/(.*\s+if\s*\(.*)/\/\/\/ \\\cond\n\1\n\/\/\/ \\\endcond/"\
#| perl -pe "s/(.*\s+else.*)/\/\/\/ \\\cond\n\1\n\/\/\/ \\\endcond/"



echo "/// \cond" | cat - $1\
| perl -00pe "s|\n\n|\n \n |smg"\
| perl -00pe "s|^(\s*\/\*\*.*?\*\/)\$|\/\/\/ \\\endcond\n\1\n\/\/\/ \\\cond|smg"
echo "/// \endcond"



#cat $1\
#| perl -00pe "s|\n\n|\n \n |smg"\
#| perl -00pe "s|^(\s+\/\*\*.*?\*\/)\$|\/\/======\n\1\n\/\/~~~~~~|smg"


