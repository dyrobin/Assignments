from collections import defaultdict
import glob
import sys
import math

directories= ["A116", "A126", "A137"]

def readToDict(fileName):
	sum=defaultdict(float)
	count=defaultdict(int)
	averages=defaultdict(float)

    	with open(fileName,"r") as text:
        	for line in text:
			values = line.split(",")
			sum[values[1]]+=float(values[5])
			count[values[1]]+=1
	for k,v in sum.items():
		averages[k]= float(v)/float(count[k])
	return averages


def makeLibrary(directoryList):
	library = defaultdict(dict)
	for dir in directoryList:
		files =  glob.glob(dir+"/*.csv")
		for file in files:
			library[file]=readToDict(file)
	return library


library = makeLibrary(directories)


locationToMatch = sys.argv[1]

print "Finding location for measurements from file "+str(file)
