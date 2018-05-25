#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys

def getCapacity(prefix):

    prefixComponents = prefix.split('/')
    if len(prefixComponents) == 1:
        return 1
    
    prefixLength = int(prefixComponents[1])

    return pow(2, 32 - prefixLength)

def countTargetsIn(filePath):
    
    if not os.path.isfile(filePath):
        return -1
    
    with open(filePath) as f:
        targets = f.read().splitlines()
    
    nbTargets = 0
    for i in range(0, len(targets)):
        nbTargets += getCapacity(targets[i])

    return nbTargets

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Usage: python CountTargets.py [AS number]")
        sys.exit()
    
    ASNumber = str(sys.argv[1])
    targetFolder = '../' + ASNumber # TODO: change the ../ part if necessary
    
    # Full target file
    fullTargetFile = targetFolder + '/' + ASNumber + ".txt"
    nbTargetsFull = countTargetsIn(fullTargetFile)
    if nbTargetsFull == -1:
        print("Target file for " + ASNumber + " could not be found. Quitting.")
        sys.exit()
    
    print("Full target file: " + str(nbTargetsFull) + " IPs")
    
    # Parts (if the folder exists)
    if os.path.isdir(targetFolder + "/Parts") == False:
        sys.exit()
    
    partsFolder = targetFolder + "/Parts"
    parts = os.listdir(partsFolder)
    parts.sort()
    total = 0
    for i in range(0, len(parts)):
        curNb = countTargetsIn(partsFolder + "/" + parts[i])
        total += curNb
        if curNb == -1:
            print("File " + parts[i] + " no longer exists.")
            continue
        print(parts[i] + ": " + str(curNb) + " IPs")
    if total != nbTargetsFull:
        print("WARNING: the sum of the parts isn't equal to the full target file.")
