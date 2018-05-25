#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import numpy as np
from matplotlib import pyplot as plt
from matplotlib import ticker

# Scans the lines of a .graph file to get the neighborhoods (identified by their labels).

def getNeighborhoods(lines):
    res = dict()
    
    for i in range(0, len(lines)):
        if not lines[i]:
            break
        
        split = lines[i].split(" - ")
        nID = split[0]
        nLabel = split[1]
        if "not among targets" in nLabel:
            secondSplit = nLabel.split(" (")
            nLabel = secondSplit[0]
        if nLabel in res:
            print("WARNING! Duplicate neighborhood: " + nLabel + " (line " + str(i + 1) + ")")
            continue
        res[nLabel] = nID

    return res

# Using the lines of the .graph file and the vertices found with getNeighborhoods, builds the 
# adjacency matrix.

def getAdjacencyMatrix(lines, vertices):
    matrix = np.zeros(shape=(len(vertices), len(vertices)))
    
    for i in range(len(vertices) + 1, len(lines)):
        if not lines[i]:
            break
        
        split = lines[i].split(' -> ')
        firstNode = split[0]
        secondNode = ""
        linkType = 1 # 1 = (in)direct link, 2 = remote link
        if "via" in split[1]:
            secondSplit = split[1].split(' via ')
            secondNode = secondSplit[0]
            if "miscellaneous" in secondSplit[1] or "anonymous" in secondSplit[1]:
                linkType = 2 # To discriminate (in)direct links from remote links
        else:
            secondSplit = split[1].split(' (')
            secondNode = secondSplit[0]
        
        firstNodeInt = int(firstNode[1:]) - 1
        secondNodeInt = int(secondNode[1:]) - 1
        matrix[firstNodeInt][secondNodeInt] = linkType
    
    return matrix

# Prepares the CDF of a degree list

def getCDF(degrees):
    if len(degrees) == 0:
        return [], []
    
    # Sort degrees and computes occurrences for each
    degrees.sort()
    totals = []
    values = []
    prevValue = -1
    accumulator = 1
    for i in range(0, len(degrees)):
        curValue = degrees[i]
        if prevValue == -1:
            prevValue = curValue
        elif prevValue == curValue:
            accumulator += 1
        else:
            totals.append(accumulator)
            values.append(prevValue)
            accumulator = 1
            prevValue = curValue
    
    # Computes ratios for each unique degree
    maxDiff = len(totals)
    ratios = []
    for i in range(0, len(totals)):
        ratios.append(float(totals[i]) / float(len(degrees)))
    
    # Builds final CDF
    CDF = []
    CDF.append(ratios[0])
    for i in range(1, len(ratios)):
        curValue = CDF[i - 1]
        curValue += ratios[i]
        CDF.append(curValue)
    
    return values, CDF

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Use this command: python NeighborhoodDegree.py [path to dataset description]")
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
    stubDegrees = []
    transitDegrees = []
    tier1Degrees = []
    dataPath = "../" # TODO: change this if necessary
    year = "2018"
    maxi = 0
    for i in range(0, len(ASes)):
        print("Processing the graph of " + ASes[i] + "...")
        dataFilePath = dataPath + "/" + ASes[i] + "/" + year + "/"
        dataFilePath += ASDates[ASes[i]] + "/" + ASes[i] + "_" + ASDates[ASes[i]]
        dataFilePath += ".graph"
        
        with open(dataFilePath) as f:
            graphLines = f.read().splitlines()
        graphVertices = getNeighborhoods(graphLines)
        graph = getAdjacencyMatrix(graphLines, graphVertices)
        
        degrees = []
        for j in range(0, graph.shape[0]):
            degree = 0
            for k in range(0, graph.shape[1]):
                if graph[j][k] > 0:
                    degree += 1
                if graph[k][j] > 0:
                    degree += 1
            degrees.append(degree)
        
        if ASTypes[ASes[i]] == 'Stub':
            stubDegrees.extend(degrees)
        elif ASTypes[ASes[i]] == 'Transit':
            transitDegrees.extend(degrees)
        elif ASTypes[ASes[i]] == 'Tier-1':
            tier1Degrees.extend(degrees)
    
    stubValues, stubCDF = getCDF(stubDegrees)
    transitValues, transitCDF = getCDF(transitDegrees)
    tier1Values, tier1CDF = getCDF(tier1Degrees)
    
    maxDegreeStub = 0
    maxDegreeTransit = 0
    maxDegreeTier1 = 0
    
    if len(stubValues) > 0:
        maxDegreeStub = stubValues[len(stubValues) - 1]
    if len(transitValues) > 0:
        maxDegreeTransit = transitValues[len(transitValues) - 1]
    if len(tier1Values) > 0:
        maxDegreeTier1 = tier1Values[len(tier1Values) - 1]
    
    maxDegree = 0
    if maxDegreeTier1 >= maxDegreeStub and maxDegreeTier1 >= maxDegreeTransit:
        maxDegree = maxDegreeTier1
    elif maxDegreeTransit >= maxDegreeTier1 and maxDegreeTransit >= maxDegreeStub:
        maxDegree = maxDegreeTransit
    else:
        maxDegree = maxDegreeStub
    
    nextPowerOfTen = 1
    while nextPowerOfTen < maxDegree:
        nextPowerOfTen *= 10
    
    # Plots result
    hfont = {'fontname':'serif',
             'fontweight':'bold',
             'fontsize':28}

    hfont2 = {'fontname':'serif',
             'fontsize':26}

    plt.figure(figsize=(13,9))
    
    plt.semilogx(stubValues, stubCDF, color='#000000', linewidth=3, label="Stub ASes")
    plt.semilogx(transitValues, transitCDF, color='#000000', linewidth=3, linestyle=':', label="Transit ASes")
    plt.semilogx(tier1Values, tier1CDF, color='#000000', linewidth=3, linestyle='--', label="Tier-1 ASes")
    plt.rcParams.update({'font.size': 20})
    
    plt.ylim([0, 1.05])
    plt.xlim([0, nextPowerOfTen])
    plt.yticks(np.arange(0, 1.1, 0.1), **hfont2)
    ax = plt.axes()
    yticks = ax.yaxis.get_major_ticks()
    yticks[0].label1.set_visible(False)
    ax.set_xscale('log')
    
    # Ticks for the X axis
    tickValues = []
    tickDisplay = []
    power = 1
    exponent = 0
    while power < maxDegree:
        power *= 10
        exponent +=1
        tickValues.append(power)
        tickDisplay.append(r"$10^{{ {:1d} }}$".format(exponent))
    plt.xticks(tickValues, tickDisplay, **hfont2)
    
    plt.ylabel('Cumulative distribution function (CDF)', **hfont)
    plt.xlabel('Neighborhood degree', **hfont)
    
    plt.grid()
    
    plt.legend(bbox_to_anchor=(0, 1.02, 1.0, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)
    
    plt.savefig("NeighborhoodDegrees.pdf")
    plt.clf()
