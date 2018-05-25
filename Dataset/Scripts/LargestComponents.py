#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import numpy as np
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Use this command: python LargestComponents.py [path to dataset description]")
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
    largestComponents = dict()
    dataPath = "../" # TODO: change this if necessary
    year = "2018"
    for i in range(0, len(ASes)):
        dataFilePath = dataPath + "/" + ASes[i] + "/" + year + "/"
        dataFilePath += ASDates[ASes[i]] + "/" + ASes[i] + "_" + ASDates[ASes[i]]
        dataFilePath += ".metrics"
        
        # Checks existence of the .metrics file
        if not os.path.isfile(dataFilePath):
            print(dataFilePath + " does not exist. Skipping to next AS on the list...")
            continue
        
        # Parses the .metrics file
        with open(dataFilePath) as f:
            metricsRaw = f.read().splitlines()
        
        biggestComponent = 0.0
        for j in range(0, len(metricsRaw)):
            if "Via gate" in metricsRaw[j]:
                toSplit = metricsRaw[j][:-2]
                split = toSplit.split('(')
                compSize = float(split[1])
                if compSize > biggestComponent:
                    biggestComponent = compSize
        
        print(ASes[i] + ": " + str(biggestComponent) + "% of the graph")
        
        parsedASes.append(ASes[i])
        largestComponents[ASes[i]] = biggestComponent
    
    # Sorts ASes by AS type
    sortedASes = []
    for i in range(0, len(ASes)):
        if ASTypes[ASes[i]] == 'Stub':
            sortedASes.append(ASes[i])
    
    for i in range(0, len(ASes)):
        if ASTypes[ASes[i]] == 'Transit':
            sortedASes.append(ASes[i])
    
    for i in range(0, len(ASes)):
        if ASTypes[ASes[i]] == 'Tier-1':
            sortedASes.append(ASes[i])
    
    # Displays in console the ASes in order
    print("ASes on the figure:")
    for i in range(0, len(sortedASes)):
        ASType = ASTypes[sortedASes[i]]
        print(str(i + 1) + " = " + sortedASes[i] + " (" + ASType + ")")
    
    # Gets components according to AS types; masks are built for the bar chart
    sortedComps1 = []
    sortedComps2 = []
    sortedComps3 = []
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Stub':
            sortedComps1.append(largestComponents[sortedASes[i]])
        else:
            sortedComps1.append(0.0)
    
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Transit':
            sortedComps2.append(largestComponents[sortedASes[i]])
        else:
            sortedComps2.append(0.0)
    
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Tier-1':
            sortedComps3.append(largestComponents[sortedASes[i]])
        else:
            sortedComps3.append(0.0)
    
    # Plots the ratios in a bar chart
    ind = np.arange(len(sortedASes))
    width = 0.9
    
    fig = plt.figure(figsize=(20,12))
    ax = fig.add_subplot(111)
    rects1 = ax.bar(ind + 0.05, sortedComps1, width, color='#EEEEEE')
    rects2 = ax.bar(ind + 0.05, sortedComps2, width, color='#AAAAAA')
    rects3 = ax.bar(ind + 0.05, sortedComps3, width, color='#333333')
    ax.autoscale(tight=True)
    ax.yaxis.grid(True)
    
    hfont = {'fontname':'serif',
             'fontweight':'bold',
             'fontsize':46}
    
    hfont2 = {'fontname':'serif',
              'fontsize':32}
    
    hfont3 = {'fontname':'serif',
              'fontsize':30}
    
    plt.ylabel('Size w.r.t. complete graph (%)', **hfont)
    plt.xlabel('Autonomous System', **hfont)
    plt.ylim([0,100])
    plt.xlim([0,len(sortedASes)])
    plt.xticks(ind + 0.5, np.arange(1, len(sortedASes) + 1, 1), **hfont2)
    plt.yticks(np.arange(0, 101, 10), **hfont3)
    
    plt.rc('font', family='serif', size=34)
    plt.legend((rects1[0], rects2[0], rects3[0]), 
               ('Stub AS', 'Transit AS', 'Tier-1 AS'), 
               bbox_to_anchor=(0, 1.02, 1.0, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("LargestComponents.pdf")
    plt.clf()
