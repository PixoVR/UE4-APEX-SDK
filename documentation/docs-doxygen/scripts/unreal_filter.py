#!/usr/bin/env python3

import re
import sys
import json


# This is from https://stackoverflow.com/questions/5454322/python-how-to-match-nested-parentheses-with-regex
def paren_matcher (n):
    # poor man's matched paren scanning, gives up
    # after n+1 levels.  Matches any string with balanced
    # parens inside; add the outer parens yourself if needed.
    # Nongreedy.
    return r"[^()]*?(?:\("*n+r"[^()]*?"+r"\)[^()]*?)*?"*n

# Check we have the right number of args
if len(sys.argv) != 2:
    print("Usage: "+sys.argv[0]+" <C++ Filename>", file=sys.stderr)
    sys.exit(1)

debug=False
#debug=True

# Get the filename from args
filename = sys.argv[1]

if debug: print("File: "+filename, file=sys.stderr)

macros = "UCLASS|UENUM|UINTERFACE|USTRUCT|UFUNCTION|UPROPERTY|UDELEGATE" # todo: |IMPLEMENT_MODULE|IMPLEMENT_GAME_MODULE"
macros2 = "UMETA"

def makeQualifiers(match):
	#debug = False
	#if debug: print(match)
	macro = match.group(2)
	if debug: print(macro)
	parts = re.match(r'('+macros+')\(\s*((?:[^\s]+[,\s]*)*)\s*\)',macro,re.M)
	#parts = re.match(r'('+macros+')\((.*)\)',macro,re.M)
	qualifier = ""
	original = match.group(2)
	#if debug: print(parts)
	if parts is None:
		#qualifier = "bad regex for %s" % (macro)
		return match.group(0)
	else:
		q = parts.group(2)
		#if (len(q.strip())==0):		# case of empty USTRUCT()
		#	return q
		q = re.sub('/\*(.*)\*/','',q)			# take out comments like /* something */
		original = re.sub('/\*(.*)\*/','',original)	# take out comments like /* something */
		if debug: print("a "+q, file=sys.stderr)
		q = re.sub('\(','{',q)
		q = re.sub('\)','}',q)
		if debug: print("b "+q, file=sys.stderr)
		q = re.sub('(\"[^:,]?[^"]+\")',r'"STRING"',q)
		if debug: print("d "+q, file=sys.stderr)
		q = re.sub('=\s*([^{}",]+)\s*(}|,)',r'="\1"\2',q)
		if debug: print("c "+q, file=sys.stderr)
		#q = re.sub('(\"[^:][^"]+\")([^:])',r'"STRING"\2',q)
		#q = re.sub('(\"[^:,]?[^",]+\")',r'"STRING"',q)
		q = re.sub('(\"[^:,]?[^"]+\")',r'"STRING"',q)
		if debug: print("d "+q, file=sys.stderr)
		q = re.sub('([\w]*)\s*=',r'"\1":',q)
		if debug: print("e "+q, file=sys.stderr)
		q = re.sub('([\w]+)\s*(}|,|$)',r'"\1":null\2',q)
		if debug: print("x "+q, file=sys.stderr)
		q = re.sub('([:,])\s',r'\1',q)
		if debug: print("y "+q, file=sys.stderr)
		q = re.sub('(,)(\"\w+\")(}$)',r'\1\2:null\3',q)
		if debug: print("z "+q, file=sys.stderr)
		q = re.sub('([^:])(\"\w+\")(}|,|$)',r'\1\2:null\3',q)
		if debug: print("f "+q, file=sys.stderr)
		q = re.sub('(/\*.*\*/)','',q);
		if debug: print("g "+q, file=sys.stderr)
		q = re.sub('([^{}:,]+):([^{}:,]+):null',r'\1:\2',q);
		if debug: print("h "+q, file=sys.stderr)
		q = '{'+q+'}'
		#print(filename, file=sys.stderr)
		if debug: print("w "+q, file=sys.stderr)
		j = json.loads(q)
		#print(j)
		k = list({ele for ele in j if j[ele] is None})
		#print(k)
		for i in k:
			qualifier += "\t\\qualifier "+i+"\n"

	return "%s/**\n\t\\brief **%s**\n%s*/" % (match.group(1), original, qualifier)


def makeMetas(match):
	if debug: print(match)
	macro = match.group(2)
	if debug: print(macro)
	parts = re.match(r'('+macros2+')\(\s*((?:[^\s]+[,\s]*)*)\s*\)',macro,re.M)
	output = ""
	#if debug: print(parts)
	if parts is None:
		#qualifier = "bad regex for %s" % (macro)
		return match.group(0)
	else:
		q = parts.group(2)
		q = re.sub('\(','{',q)
		q = re.sub('\)','}',q)
		if debug: print("0 "+q, file=sys.stderr)
		q = re.sub('=\s*([^",]+)\s*(,|$)',r'="\1"\2',q)
		if debug: print("1 "+q, file=sys.stderr)
		#q = re.sub('(\"[^:][^"]+\")([^:])',r'"STRING"\2',q)
		#q = re.sub('(\"[^:,]?[^",]+\")',r'"STRING"',q)
		q = re.sub('(\"[^:,]?[^"]+\")',r'"STRING"',q)
		if debug: print("2 "+q, file=sys.stderr)
		q = re.sub('([\w]*)\s*=',r'"\1":',q)
		#q = re.sub('([^\W\=]*)\s*=',r'"\1":',q)
		if debug: print("3 "+q, file=sys.stderr)
		#q = re.sub('"?([\w]+)"?\s*(}|,|$)',r'"\1":null\2',q)
		#q = re.sub('"?([\w]+)"?\s*(}|,|$)',r'"\1":null\2',q)
		q = re.sub('(?<!:)\s"([^":]+)"\s*(}|,|$)',r'"\1":null\2',q)
		if debug: print("4 "+q, file=sys.stderr)
		q = re.sub(r'^(["\'][^":]*["\'])$',r'\1:null',q)
		if debug: print("5 "+q, file=sys.stderr)
		q = re.sub('(/\*.*\*/)','',q)
		if debug: print("6 "+q, file=sys.stderr)
		q = re.sub('^([^"]*)$',r'"\1":null',q)
		if debug: print("7 "+q, file=sys.stderr)
		q = '{'+q+'}'
		#print(filename, file=sys.stderr)
		if debug: print(q, file=sys.stderr)
		j = json.loads(q)
		if debug: print("===========")
		if debug: print(j)
		k = list({ele for ele in j if j[ele] is None})
		for i in j:
			if debug: print("i: "+i)
			if j[i] is None:
				output += "<br/>**"+i+"**"
			else:
				output += "<br/>*"+i+"*: **"+j[i]+"**"
		if debug: print("output: "+output)
		if output:
			output = "Metadata: "+output

	return "%s/** %s **/" % (match.group(1), output)


# Do a regular expression to replace all UE4 macros, include balanced params

try:
	# Slurp file into a single string
	file = open(filename, 'r')
	if file.closed:
	    print("Cannot read file", file=sys.stderr)
	    sys.exit(1)
	content = file.read()

	regex = '^(\s*)((?:'+macros+')\s*\('+paren_matcher(25)+'\))'
	content = re.sub(regex, makeQualifiers, content, flags=re.MULTILINE)

	regex2 = '(\s*)((?:'+macros2+')\s*\('+paren_matcher(25)+'\))'
	content = re.sub(regex2, makeMetas, content, flags=re.MULTILINE)

	content = re.sub("(GENERATED.*DY)\(\)\r?\n",r"\1();\n",content, flags=re.MULTILINE)
except Exception as e:
	print("Error on: "+filename, file=sys.stderr)
	raise


# Output the content
print(content, end='')
