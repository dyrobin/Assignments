from collections import defaultdict
import glob
import sys
import math
import operator


directories = ["A116", "A126", "A137", "A141"]
defaultSS = -99

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

def unifyDicts(dictA,dictB):
	separation = dictA.viewkeys() - dictB.viewkeys()
	for key in separation:
		 dictB[key]=defaultSS

	separation = dictB.viewkeys() - dictA.viewkeys()
	for key in separation:
		 dictA[key]=defaultSS
	return ( dictA, dictB)

def euclideanDistance(locationToMatch, referencePoint):
	euclideanSum=0.0
	unifiedDicts = unifyDicts(readToDict(locationToMatch),referencePoint)
	dictA=unifiedDicts[0]
	dictB=unifiedDicts[1]
	for key in dictA:
		euclideanSum+=float(math.pow((dictA[key]-dictB[key]),2))
	return float(math.sqrt(euclideanSum))

library = makeLibrary(directories)
locationToMatch = sys.argv[1]
distances = defaultdict(float)

print "Finding location for measurements from file "+str(locationToMatch)



for key in sorted(library.iterkeys()):
	distances[key]=euclideanDistance(locationToMatch, library[key])
	print key+" distance "+str(round(distances[key],2))
	

