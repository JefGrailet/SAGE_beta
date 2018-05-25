#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import numpy as np
from matplotlib import pyplot as plt
from matplotlib import ticker as ticker

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Use this command: python SubnetsAsLinks.py [path to dataset description]")
        sys.exit()
    
    datasetPath = str(sys.argv[1])
    
    # Parses dataset file
    if not os.path.isfile(datasetPath):
        print("Dataset file does not exist")
        sys.exit()

    with open(datasetPath) as f:
        datasetRaw = f.read().splitlines()
    
    ASes = []
    ASTypes = dict()
    ASDates = dict()
    for i in range(0, len(datasetRaw)):
        splitted = datasetRaw[i].split(':')
        ASes.append(splitted[0])
        ASTypes[splitted[0]] = splitted[1]
        ASDates[splitted[0]] = splitted[2]
    ASes.sort()

    # Analyses each AS when possible
    parsedASes = [] # To have a list of successfully parsed ASes
    parsedSubnets = dict()
    prefixLengths = np.zeros((12,))
    prefixLengthsPerAS = []
    for i in range(0, 12):
        prefixLengthsPerAS.append(dict())
    
    dataPath = "../" # TODO: change this if necessary
    year = "2018"
    for i in range(0, len(ASes)):
        dataFilePath = dataPath + "/" + ASes[i] + "/" + year + "/"
        dataFilePath += ASDates[ASes[i]] + "/" + ASes[i] + "_" + ASDates[ASes[i]]
        dataFilePath += ".graph"
        
        # Checks existence of the .neighborhoods file
        if not os.path.isfile(dataFilePath):
            print(dataFilePath + " does not exist. Skipping to next AS on the list...")
            continue
        
        # Parses the .graph file
        with open(dataFilePath) as f:
            graphRaw = f.read().splitlines()
        
        part = 0
        for j in range(0, len(graphRaw)):
            if not graphRaw[j]:
                part += 1
            elif part == 1:
                if "via" in graphRaw[j] and "/" in graphRaw[j]:
                    split1 = graphRaw[j].split(' via ')
                    subnet = split1[1]
                    if " (" in subnet:
                        split2 = subnet.split(' (')
                        subnet = split2[0]
                    if subnet not in parsedSubnets:
                        parsedSubnets[subnet] = 1
                        split3 = subnet.split('/')
                        prefixLen = int(split3[1])
                        prefixLengths[(prefixLen - 20)] += 1
                        if ASes[i] not in prefixLengthsPerAS[(prefixLen - 20)]:
                            prefixLengthsPerAS[(prefixLen - 20)][ASes[i]] = 0
                        prefixLengthsPerAS[(prefixLen - 20)][ASes[i]] += 1
                    else:
                        parsedSubnets[subnet] += 1
    
    maxValue = 0
    for i in range(0, 12):
        if prefixLengths[i] > maxValue:
            maxValue = prefixLengths[i]
    
    print("Amount of subnets for each prefix length:")
    for i in range(0, 12):
        print("\n/" + str(i + 20) + " subnets:")
        for AS in prefixLengthsPerAS[i]:
            print(AS + " => " + str(prefixLengthsPerAS[i][AS]))
    
    # Plots results (amount of subnets for each prefix length)
    hfont = {'fontname':'serif',
             'fontweight':'bold',
             'fontsize':28}
    
    hfont2 = {'fontname':'serif',
              'fontsize':22}
    
    hfont3 = {'fontname':'serif',
              'fontsize':22}
    
    plt.figure(figsize=(18, 12))
    
    ind = np.arange(12)
    width = 0.90
    
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.autoscale(tight=True)
    ax.set_yscale('log')
    rects = ax.bar(ind + 0.05, prefixLengths, width, color='#AAAAAA')
    
    plt.ylabel("Amount of subnets", **hfont)
    plt.xlabel("Prefix", **hfont)
    
    # Ticks for the X axis (always the same)
    positions = [0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5]
    labels = ["/20", "/21", "/22", "/23", "/24", "/25", 
              "/26", "/27", "/28", "/29", "/30", "/31"]
    
    plt.subplots_adjust(bottom=0.13)
    
    # Ticks for the Y axis
    tickValues = []
    tickDisplay = []
    power = 100
    exponent = 2
    while power < maxValue:
        tickValues.append(power)
        tickDisplay.append(r"$10^{{ {:1d} }}$".format(exponent))
        power *= 10
        exponent +=1
    
    # Sets the ticks
    plt.yticks(tickValues, tickDisplay, **hfont2)
    plt.xticks(positions, labels, **hfont3)
    plt.xlim([0, 12])
    
    plt.savefig("SubnetsAsLinks.pdf")
    plt.clf()
    
    # Computes average amount of implemented links
    accumulators = np.zeros((12,))
    for subnet in parsedSubnets:
        split = subnet.split('/')
        prefixLen = int(split[1])
        nbLinks = parsedSubnets[subnet]
        accumulators[(prefixLen - 20)] += nbLinks
    
    averages = np.zeros((12,), dtype=np.float64)
    for i in range(0, 12):
        averages[i] = float(accumulators[i]) / float(prefixLengths[i])
    
    maxValueBis = 0.0
    for i in range(0, 12):
        if averages[i] > maxValueBis:
            maxValueBis = averages[i]
    
    # Average amount of implemented links per prefix length
    plt.figure(figsize=(18, 12))
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.autoscale(tight=True)
    ax.yaxis.grid(True)
    rects = ax.bar(ind + 0.05, averages, width, color='#AAAAAA')
    
    plt.ylabel("Implemented links (avg)", **hfont)
    plt.xlabel("Prefix", **hfont)
    
    plt.subplots_adjust(bottom=0.13)
    plt.yticks(np.arange(0, 21, 2), **hfont3)
    plt.xticks(positions, labels, **hfont3)
    plt.xlim([0, 12])
    plt.ylim([0, 20])
    
    plt.savefig("AverageSubnetUsage.pdf")
    plt.clf()
