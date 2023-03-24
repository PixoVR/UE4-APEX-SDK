#!/usr/bin/env python3

import re
import sys
import json

# Get the filename from args
filename = sys.argv[1]

# Slurp file into a single string
file = open(filename, 'r')
if file.closed:
    print("Cannot read file", file=sys.stderr)
    sys.exit(1)
content = file.read()

def makeQualifier(match):
	#print(match)
	decorator = match.group(2)
	#decorator = decorator.replace(".","\\\\\.")

	return "%s/** \\brief **%s**\n%s    \\qualifier \"%s\"\n%s*/\n%s" % (match.group(1), match.group(0).strip(), match.group(1), decorator, match.group(1), match.group(0))


# Do a regular expression to replace all UE4 macros, include balanced params
decorators = "System\.Serializable|SerializeField|Serializable"
regex = '^(\s*)\[\s*('+decorators+')\s*\]'

content = re.sub(regex, makeQualifier, content, flags=re.MULTILINE)

# Output the content
print(content, end='')
