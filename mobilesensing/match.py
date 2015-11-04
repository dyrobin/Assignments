from collections import defaultdict
import glob
import sys
import math
import operator

# Dictionary (key = BSSID, value = signal level)
def dictReadFromFile(file):
    sum = defaultdict(float)
    count = defaultdict(int)
    avrg = defaultdict(float)

    with open(file, "r") as text:
        for line in text:
            values = line.split(",")
            sum[values[1]] += float(values[5])
            count[values[1]] += 1

    for k,v in sum.items():
        avrg[k] = float(v) / float(count[k])
    
    return avrg

def dictReadFromFolder(folder):
    sum = defaultdict(float)
    count = defaultdict(int)
    avrg = defaultdict(float)

    files = glob.glob(folder + "/*.csv")
    for file in files:
        with open(file, "r") as text:
            for line in text:
                values = line.split(",")
                sum[values[1]] += float(values[5])
                count[values[1]] += 1

    for k,v in sum.items():
        avrg[k]= float(v)/float(count[k])

    return avrg

# Reference Data generated from a file, i.e. a single measurement
def refdataGenFromFile(folders):
    refdata = defaultdict(dict)
    for folder in folders:
        files =  glob.glob(folder + "/*.csv")
        for file in files:
            refdata[file] = dictReadFromFile(file)

    return refdata

# Reference Data generated from a folder, i.e. multiple measurements
def refdataGenFromFolder(folders):
    refdata = defaultdict(dict)
    for folder in folders:
        refdata[folder] = dictReadFromFolder(folder);

    return refdata

# Make two dictionaries have same keys. 
def unifyDicts(dictA, dictB):

    return (dictA, dictB)

def euclideanDistance(dictA, dictB):
    # Before computing distance between two dictionaries, make sure they have
    # same keys. If a key is in dictA but not in dictB, fill it using default 
    # signal level and vice versa.
    dftlevel = -99
    diffkeys = dictA.viewkeys() - dictB.viewkeys()
    for key in diffkeys:
         dictB[key] = dftlevel

    diffkeys = dictB.viewkeys() - dictA.viewkeys()
    for key in diffkeys:
         dictA[key] = dftlevel

    # computing euclidean distance
    euclideanSum = 0.0
    for key in dictA:
        euclideanSum += float(math.pow((dictA[key]-dictB[key]), 2))
    return float(math.sqrt(euclideanSum))


folders = ["A112", "A118", "A124", "A130", "A136", "A141"]

refdata1 = refdataGenFromFile(folders)
refdata2 = refdataGenFromFolder(folders)

toMatch = dictReadFromFile(sys.argv[1])


print "To match location by single measurement......"
distanceMin = 100.00
keyMin = "No Place"
for key in refdata1.iterkeys():
    distance = euclideanDistance(toMatch, refdata1[key])
    print key + ": " + str(round(distance, 2))
    if distance < distanceMin:
        distanceMin = distance
        keyMin = key
print "Closest place: "+ keyMin + " (" + str(round(distanceMin, 2)) + ")"

print ""

print "To match location by multiple measurements......"
distanceMin = 100.00
keyMin = "No Place"
for key in refdata2.iterkeys():
    distance = euclideanDistance(toMatch, refdata2[key])
    print key +": "+ str(round(distance, 2))
    if distance < distanceMin:
        distanceMin = distance
        keyMin = key
print "Closest place: "+ keyMin + " (" + str(round(distanceMin, 2)) + ")"
