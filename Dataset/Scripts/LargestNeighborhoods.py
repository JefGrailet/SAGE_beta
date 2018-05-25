#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
import numpy as np
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 2:
        print("Use this command: python LargestNeighborhoods.py [path to dataset description]")
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
    largestNeighborhoods = dict()
    largestNeighborhoodsType = dict()
    superNb = dict()
    trueMax = 0
    dataPath = "../" # TODO: change this if necessary
    year = "2018"
    for i in range(0, len(ASes)):
        dataFilePath = dataPath + "/" + ASes[i] + "/" + year + "/"
        dataFilePath += ASDates[ASes[i]] + "/" + ASes[i] + "_" + ASDates[ASes[i]]
        dataFilePath += ".neighborhoods"
        
        # Checks existence of the .neighborhoods file
        if not os.path.isfile(dataFilePath):
            print(dataFilePath + " does not exist. Skipping to next AS on the list...")
            continue
        
        # Parses the .neighborhoods file
        with open(dataFilePath) as f:
            neighborhoodsRaw = f.read().splitlines()
        
        isCluster = False
        currentNeighborhood = 0
        biggestNode = 0
        biggestCluster = 0
        for j in range(0, len(neighborhoodsRaw)):
            if not neighborhoodsRaw[j]:
                if currentNeighborhood >= 200:
                    if ASes[i] not in superNb:
                        superNb[ASes[i]] = 0
                    superNb[ASes[i]] += 1
                if isCluster:
                    if currentNeighborhood > biggestCluster:
                        biggestCluster = currentNeighborhood
                else:
                    if currentNeighborhood > biggestNode:
                        biggestNode = currentNeighborhood
                isCluster = False
                currentNeighborhood = 0
            else:
                if "Node" in neighborhoodsRaw[j] or "Cluster" in neighborhoodsRaw[j]:
                    if "Cluster" in neighborhoodsRaw[j]:
                        isCluster = True
                else:
                    currentNeighborhood += 1
        
        # Prints the data for the current AS
        print(ASes[i] + ":")
        print("Largest node: " + str(biggestNode))
        if biggestCluster > 0:
            print("Largest cluster: " + str(biggestCluster))
        else:
            print("No cluster.")
        
        parsedASes.append(ASes[i])
        curMax = biggestNode
        isMaxCluster = False
        if biggestCluster > curMax:
            curMax = biggestCluster
            isMaxCluster = True
        
        largestNeighborhoods[ASes[i]] = curMax
        largestNeighborhoodsType[ASes[i]] = isMaxCluster
        
        if curMax > trueMax:
            trueMax = curMax
    
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
    
    # Gets the maxima and total of super neighborhoods (>200 subnets) per AS type
    sortedMax1 = []
    sortedNb1 = []
    sortedMax2 = []
    sortedNb2 = []
    sortedMax3 = []
    sortedNb3 = []
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Stub':
            sortedMax1.append(largestNeighborhoods[sortedASes[i]])
            if sortedASes[i] in superNb:
                sortedNb1.append(superNb[sortedASes[i]])
            else:
                sortedNb1.append(0)
            if largestNeighborhoodsType[sortedASes[i]]:
                print("Largest neighborhood in " + sortedASes[i] + " is a cluster.")
        else:
            sortedMax1.append(0)
            sortedNb1.append(0)
    
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Transit':
            sortedMax2.append(largestNeighborhoods[sortedASes[i]])
            if sortedASes[i] in superNb:
                sortedNb2.append(superNb[sortedASes[i]])
            else:
                sortedNb2.append(0)
            if largestNeighborhoodsType[sortedASes[i]]:
                print("Largest neighborhood in " + sortedASes[i] + " is a cluster.")
        else:
            sortedMax2.append(0)
            sortedNb2.append(0)
    
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Tier-1':
            sortedMax3.append(largestNeighborhoods[sortedASes[i]])
            if sortedASes[i] in superNb:
                sortedNb3.append(superNb[sortedASes[i]])
            else:
                sortedNb3.append(0)
            if largestNeighborhoodsType[sortedASes[i]]:
                print("Largest neighborhood in " + sortedASes[i] + " is a cluster.")
        else:
            sortedMax3.append(0)
            sortedNb3.append(0)
    
    # Plots the max. sizes in a bar chart
    ind = np.arange(len(sortedASes))
    width = 0.9
    
    fig = plt.figure(figsize=(20,12))
    ax = fig.add_subplot(111)
    rects1 = ax.bar(ind + 0.05, sortedMax1, width, color='#EEEEEE')
    rects2 = ax.bar(ind + 0.05, sortedMax2, width, color='#AAAAAA')
    rects3 = ax.bar(ind + 0.05, sortedMax3, width, color='#333333')
    ax.autoscale(tight=True)
    ax.yaxis.grid(True)
    
    hfont = {'fontname':'serif',
             'fontweight':'bold',
             'fontsize':46}
    
    hfont2 = {'fontname':'serif',
              'fontsize':32}
    
    hfont3 = {'fontname':'serif',
              'fontsize':30}
    
    hfont4 = {'fontname':'serif',
              'fontsize':34}
    
    # Writes the total amount of clusters on top of each bar
    for rect, label in zip(rects1, sortedNb1):
        if label == 0:
            continue
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height + 0.5, label, ha='center', va='bottom', **hfont4)
    
    for rect, label in zip(rects2, sortedNb2):
        if label == 0:
            continue
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height + 0.5, label, ha='center', va='bottom', **hfont4)
    
    for rect, label in zip(rects3, sortedNb3):
        if label == 0:
            continue
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height + 0.5, label, ha='center', va='bottom', **hfont4)
    
    plt.ylabel('Amount of aggregated subnets', **hfont)
    plt.xlabel('Autonomous System', **hfont)
    plt.ylim([0, (int((float(trueMax) / 10) * 11) / 1000) * 1000 + 500])
    plt.xlim([0, len(sortedASes)])
    plt.xticks(ind + 0.5, np.arange(1, len(sortedASes) + 1, 1), **hfont2)
    plt.yticks(np.arange(0, int((float(trueMax) / 10) * 11), 1000), **hfont3)
    
    plt.rc('font', family='serif', size=34)
    plt.legend((rects1[0], rects2[0], rects3[0]), 
               ('Stub AS', 'Transit AS', 'Tier-1 AS'), 
               bbox_to_anchor=(0, 1.02, 1.0, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("LargestNeighborhoods.pdf")
    plt.clf()
