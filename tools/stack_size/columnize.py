#!/usr/bin/python
#
# columnize - simple program to produce columnar output from
# whitespace-separated lines

import fileinput

rows=[]
col_maxes = {}
# gather input into tuples, and find max column widths
for line in fileinput.input():
	r = line.split()
	# accumulate max values
	col = 0
	for item in r:
		max = col_maxes.get(col, 0)
		if len(item)>max:
			col_maxes[col] = len(item)
		col += 1
	rows.append(r)

#for col in range(0,4):
#	print "col_maxes[%d]=%d" % (col, col_maxes[col])
col_maxes[0] = 30

# output data in with column-width spacing
for row in rows:
	col = 0
	for i in row:
		print "%s%s" % (i, " "*(col_maxes[col]-len(i))),
		col += 1
	print

