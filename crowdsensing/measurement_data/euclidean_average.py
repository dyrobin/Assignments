from collections import defaultdict
import glob
import sys
import math
import operator


directories = [ "A112", "A118", "A124", "A130", "A136", "A141" ]
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

def readToDictFolder(folder):
        sum=defaultdict(float)
        count=defaultdict(int)
        averages=defaultdict(float)

	#print "readToDictFolder: calculations for folder " + folder

	files =  glob.glob(folder+"/*.csv")
	for fileName in files:
		with open(fileName,"r") as text:
                	for line in text:
                        	values = line.split(",")
                        	sum[values[1]]+=float(values[5])
                        	count[values[1]]+=1
        for k,v in sum.items():
                averages[k]= float(v)/float(count[k])
		#print "readToDictFolder: Sum of lines for calculation for key "+ k +" "+ str(count[k]) + ", average "+ str(averages[k])
        return averages


def makeLibrary(directoryList):
	library = defaultdict(dict)
	for dir in directoryList:
		files =  glob.glob(dir+"/*.csv")
		for file in files:
			library[file]=readToDict(file)
	return library

def makeLibraryFromFolders(directoryList):
	library = defaultdict(dict)
	for dir in directoryList:
		library[dir]=readToDictFolder(dir)
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
libraryLocationAvg = makeLibraryFromFolders(directories)
locationToMatch = sys.argv[1]

#print "Finding location for measurements from file "+str(locationToMatch)


distanceMin=100.00
keyMin="noKey"

del library[locationToMatch]

for key in sorted(library.iterkeys()):
	distance=euclideanDistance(locationToMatch, library[key])
	#print key+" distance "+str(round(distances[key],2))
	if distance < distanceMin:
		distanceMin=distance
		keyMin=key

print "Closest location for "+locationToMatch+" measurement is "+(keyMin.split("/"))[0]+" distance "+str(round(distanceMin))

distanceMin=100.00

for key in sorted(libraryLocationAvg.iterkeys()):
	distance=euclideanDistance(locationToMatch, libraryLocationAvg[key])
	#print key+" distance "+str(round(distances[key],2))
	if distance < distanceMin:
		distanceMin=distance
		keyMin=key
print "Closest location for "+locationToMatch+" measurement is "+str(keyMin)+" distance "+str(round(distanceMin)) + " (averaging)"

