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

# Output the content
print(content, end='')
