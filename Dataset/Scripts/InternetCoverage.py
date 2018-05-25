#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os

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
    
    subDirs = []
    for f in os.listdir('../'): # TODO: change the "../" if necessary
        if os.path.isdir('../' + f) and f.startswith("AS"): # TODO: change the "../" if necessary
            subDirs.append(f)
    subDirs.sort()
    
    total = 0
    for i in range(0, len(subDirs)):
        targetFilePath = '../' + subDirs[i] + '/' + subDirs[i] + '.txt' # TODO: change the "../" if necessary
        curNb = countTargetsIn(targetFilePath)
        total += curNb
        print(subDirs[i] + ": " + str(curNb) + " IPs")
    
    IPv4Scope = pow(2, 32)
    ratio = (float(total) / float(IPv4Scope)) * 100
    print("Total of target IPs: " + str(total))
    print("Ratio w.r.t. IPv4 scope: " + str('%3f' % ratio) + "%")
