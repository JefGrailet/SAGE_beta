#! /usr/bin/env python
# -*- coding: utf-8 -*-

# For each dataset of a given date for a given list of ASes, parses the penultimate hops for each 
# subnet, then evaluate, for each route, how far is the next IP that is also a penultimate hop (if 
# any).

import os
import sys
import numpy as np
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 4:
        print("Use this command: python PeersDistance.py [year] [date] [path to AS file]")
        sys.exit()
    
    yearOfMeasurements = str(sys.argv[1])
    dateOfMeasurements = str(sys.argv[2])
    ASFilePath = str(sys.argv[3])
    
    # Parses AS file
    if not os.path.isfile(ASFilePath):
        print("AS file does not exist")
        sys.exit()

    with open(ASFilePath) as f:
        ASesRaw = f.read().splitlines()
        
    # For this particular file, we do not class by type. We remove the :[type] part.
    ASes = []
    for i in range(0, len(ASesRaw)):
        splitted = ASesRaw[i].split(':')
        ASes.append(splitted[0])

    # Analyses each AS when possible
    differencesInTTL = []
    totalOfRoutes = 0
    dataPath = "./"
    for i in range(0, len(ASes)):
        dataFilePath = dataPath + "/" + ASes[i] + "/" + yearOfMeasurements + "/"
        dataFilePath += dateOfMeasurements + "/" + ASes[i] + "_" + dateOfMeasurements
        dataFilePath += ".subnets"
        
        # Checks existence of the file
        if not os.path.isfile(dataFilePath):
            print(dataFilePath + " does not exist. Skipping to next AS on the list...")
            continue
        
        # Parses and formats subnets
        with open(dataFilePath) as f:
            subnetsRaw = f.read().splitlines()
        
        formattedSubnets = []
        tempSubnet = []
        lineCount = 0
        for j in range(0, len(subnetsRaw)):
            if subnetsRaw[j] == '':
                formattedSubnets.append(tempSubnet)
                tempSubnet = []
                lineCount = 0
            else:
                tempSubnet.append(subnetsRaw[j])
                lineCount += 1
        
        for j in range(0, len(formattedSubnets)):
            route = formattedSubnets[j][3].split(", ")
            for k in range(0, len(route)):
                if route[k] == 'Anonymous' or route[k] == 'Missing':
                    route[k] = '0.0.0.0'
                elif '[' in route[k]:
                    splitted = route[k].split(" [")
                    route[k] = splitted[0]
            formattedSubnets[j][3] = route
        
        # Parses the unique penultimate hops
        penultimateHops = set()
        for j in range(0, len(formattedSubnets)):
            route = formattedSubnets[j][3]
            IP = route[len(route) - 1]
            if IP != '0.0.0.0' and IP not in penultimateHops:
                penultimateHops.add(IP)
        
        # Checks in each route how far is the next IP that is also a penultimate hop
        for j in range(0, len(formattedSubnets)):
            route = formattedSubnets[j][3]
            if route[len(route) - 1] == '0.0.0.0':
                continue
            for k in range(1, len(route) - 1):
                nextIP = route[len(route) - 1 - k]
                if nextIP in penultimateHops:
                    differencesInTTL.append(k);
                    break
        
        totalOfRoutes += len(formattedSubnets)
    
    differencesInTTL.sort()
    totals = []
    prevValue = 0
    accumulator = 1
    for i in range(0, len(differencesInTTL)):
        curValue = differencesInTTL[i]
        if prevValue == 0:
            prevValue = curValue
        elif prevValue == curValue:
            accumulator += 1
        else:
            totals.append(accumulator)
            accumulator = 1
            prevValue = curValue
    
    maxDiff = len(totals)
    ratios = []
    for i in range(0, len(totals)):
        ratios.append(float(totals[i]) / float(totalOfRoutes))
    
    xAxis = np.arange(0, maxDiff + 1, 1)
    CDF = []
    CDF.append(0)
    for i in range(0, len(ratios)):
        curValue = CDF[i]
        curValue += ratios[i]
        CDF.append(curValue)
    
    print("Total amount of parsed routes: " + str(totalOfRoutes))
    
    # Plots result
    hfont = {'fontname':'serif',
             'fontweight':'bold',
             'fontsize':21}

    hfont2 = {'fontname':'serif',
             'fontsize':17}

    plt.figure(figsize=(13,9))
    
    plt.ylim([0,1])
    plt.xlim([0,maxDiff])
    plt.plot(xAxis, CDF, color='#000000', linewidth=3)
    plt.xticks(np.arange(0, maxDiff + 1, 1), **hfont2)
    plt.yticks(np.arange(0, 1.1, 0.1), **hfont2)
    ax = plt.axes()
    yticks = ax.yaxis.get_major_ticks()
    yticks[0].label1.set_visible(False)
    plt.ylabel('Cumulative density function (CDF)', **hfont)
    plt.xlabel('Distance in TTL between last hops and their peers', **hfont)
    plt.grid()

    plt.savefig("Peers_distance.pdf")
    plt.clf()
