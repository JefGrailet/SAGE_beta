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
    
    nbClusters = dict()
    maxima = dict()
    averages = dict()
    trueMaxPeers = 0
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
        
        curCluster = "" # Only for displaying if wished
        curNbClusters = 0
        inCluster = False
        inPeersList = False
        curInDegree = 0
        maxInDegree = 0
        totalPeers = 0
        for j in range(0, len(neighborhoodsRaw)):
            if inCluster and not neighborhoodsRaw[j]:
                inCluster = False
                inPeersList = False
                # print(curCluster + " = " + str(curInDegree))
                totalPeers += curInDegree
                if curInDegree > maxInDegree:
                    maxInDegree = curInDegree
                curInDegree = 0
                continue
            if "Cluster" in neighborhoodsRaw[j]:
                toSplit = ""
                if "only one" in neighborhoodsRaw[j]: # For accurate display only
                    toSplit = neighborhoodsRaw[j][:-22]
                else:
                    toSplit = neighborhoodsRaw[j][:-2]
                split = toSplit.split('{')
                curCluster = split[1]
                inCluster = True
                curNbClusters += 1
            if inCluster:
                if "Peer" in neighborhoodsRaw[j]:
                    if "Peers" in neighborhoodsRaw[j]:
                        inPeersList = True
                    else:
                        curInDegree = 1
                elif inPeersList:
                    curInDegree += 1
        
        avgPeers = 0.0
        if curNbClusters > 0:
            avgPeers = float(totalPeers) / float(curNbClusters)
        
        maxima[ASes[i]] = maxInDegree
        nbClusters[ASes[i]] = curNbClusters
        averages[ASes[i]] = avgPeers
        
        print(ASes[i] + ":")
        if curNbClusters > 0:
            print("Maximum amount of peers: " + str(maxInDegree))
            print("Average amount of peers: " + str(avgPeers))
        else:
            print("No cluster.")
        
        if maxInDegree > trueMaxPeers:
            trueMaxPeers = maxInDegree
    
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
    
    # Gets maxima and average cluster entering degree per AS type
    sortedMax1 = []
    sortedAvg1 = []
    sortedMax2 = []
    sortedAvg2 = []
    sortedMax3 = []
    sortedAvg3 = []
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Stub':
            sortedMax1.append(maxima[sortedASes[i]])
            sortedAvg1.append("%.1f" % averages[sortedASes[i]])
        else:
            sortedMax1.append(0)
            sortedAvg1.append(0)
    
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Transit':
            sortedMax2.append(maxima[sortedASes[i]])
            sortedAvg2.append("%.1f" % averages[sortedASes[i]])
        else:
            sortedMax2.append(0)
            sortedAvg2.append(0)
    
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Tier-1':
            sortedMax3.append(maxima[sortedASes[i]])
            sortedAvg3.append("%.1f" % averages[sortedASes[i]])
        else:
            sortedMax3.append(0)
            sortedAvg3.append(0)
    
    # Plots the max. degree of each AS as a bar chart
    ind = np.arange(len(sortedASes))
    width = 0.9
    
    fig = plt.figure(figsize=(18,12))
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
              'fontsize':28}
    
    # Writes the average amount of peers on top of each bar
    for rect, label in zip(rects1, sortedAvg1):
        if label == "0.0" or float(label) == 0.0:
            continue
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height + 0.5, label, ha='center', va='bottom', **hfont4)
    
    for rect, label in zip(rects2, sortedAvg2):
        if label == "0.0" or float(label) == 0.0:
            continue
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height + 0.5, label, ha='center', va='bottom', **hfont4)
    
    for rect, label in zip(rects3, sortedAvg3):
        if label == "0.0" or float(label) == 0.0:
            continue
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height + 0.5, label, ha='center', va='bottom', **hfont4)
    
    plt.ylabel('Entering degree of clusters', **hfont)
    plt.xlabel('Autonomous System', **hfont)
    plt.ylim([0, int((float(trueMaxPeers) / 10) * 11)])
    plt.xlim([0, len(sortedASes)])
    plt.xticks(ind + 0.5, np.arange(1, len(sortedASes) + 1, 1), **hfont2)
    plt.yticks(np.arange(0, int((float(trueMaxPeers) / 10) * 11) + 1, 5), **hfont3)
    
    plt.rc('font', family='serif', size=34)
    plt.legend((rects1[0], rects2[0], rects3[0]), 
               ('Stub AS', 'Transit AS', 'Tier-1 AS'), 
               bbox_to_anchor=(0, 1.02, 1.0, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("InDegreeClusters.pdf")
    plt.clf()
