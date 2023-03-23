#!/bin/bash

echo "/// \cond" | cat - $1\
| perl -00pe "s|\n\n|\n \n |smg"\
| perl -00pe "s|^(\s*\/\*\*.*?\*\/)\$|\/\/\/ \\\endcond\n\1\n\/\/\/ \\\cond|smg"
echo "/// \endcond"



