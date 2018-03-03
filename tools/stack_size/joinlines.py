#!/usr/bin/python

import fileinput

first_line = ""
for line in fileinput.input():
	if not first_line:
		first_line=line
	else:
		print first_line.rstrip(), line.rstrip()
		first_line=""

