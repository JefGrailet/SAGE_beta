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
    
    averages = dict()
    nbClusters = dict()
    maxima = dict()
    trueMax = 0
    dataPath = "/home/jefgrailet/PhD/Campaigns/SAGE"
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
        
        clusters = []
        for j in range(0, len(neighborhoodsRaw)):
            if "Cluster" in neighborhoodsRaw[j]:
                toSplit = neighborhoodsRaw[j][:-2]
                split = toSplit.split('{')
                IPs = split[1].split(', ')
                clusters.append(len(IPs))
        
        maxCluster = 0
        acc = 0
        for j in range(0, len(clusters)):
            acc += clusters[j]
            if clusters[j] > maxCluster:
                maxCluster = clusters[j]
        
        avg = 0.0
        if len(clusters) > 0:
            avg = float(acc) / float(len(clusters))
        
        maxima[ASes[i]] = maxCluster
        nbClusters[ASes[i]] = len(clusters)
        averages[ASes[i]] = avg
        
        print(ASes[i] + ":")
        if len(clusters) > 0:
            print("Largest cluster: " + str(maxCluster))
            print("Average cluster size: " + str(avg))
        else:
            print("No cluster.")
        
        if maxCluster > trueMax:
            trueMax = maxCluster
    
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
    
    print(sortedASes)
    
    # Gets components according to AS types; masks are built for the bar chart
    sortedMax1 = []
    sortedNb1 = []
    sortedMax2 = []
    sortedNb2 = []
    sortedMax3 = []
    sortedNb3 = []
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Stub':
            sortedMax1.append(maxima[sortedASes[i]])
            sortedNb1.append(nbClusters[sortedASes[i]])
        else:
            sortedMax1.append(0)
            sortedNb1.append(0)
    
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Transit':
            sortedMax2.append(maxima[sortedASes[i]])
            sortedNb2.append(nbClusters[sortedASes[i]])
        else:
            sortedMax2.append(0)
            sortedNb2.append(0)
    
    for i in range(0, len(sortedASes)):
        if ASTypes[sortedASes[i]] == 'Tier-1':
            sortedMax3.append(maxima[sortedASes[i]])
            sortedNb3.append(nbClusters[sortedASes[i]])
        else:
            sortedMax3.append(0)
            sortedNb3.append(0)
    
    # Plots in a bar chart (4 bars per AS) the ratios
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
             'fontsize':32}
    
    hfont2 = {'fontname':'serif',
              'fontsize':21}
    
    hfont3 = {'fontname':'serif',
              'fontsize':26}
    
    # Writes the total amount of clusters on top of each bar
    for rect, label in zip(rects1, sortedNb1):
        if label == 0:
            continue
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height + 0.5, label, ha='center', va='bottom', **hfont3)
    
    for rect, label in zip(rects2, sortedNb2):
        if label == 0:
            continue
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height + 0.5, label, ha='center', va='bottom', **hfont3)
    
    for rect, label in zip(rects3, sortedNb3):
        if label == 0:
            continue
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width() / 2, height + 0.5, label, ha='center', va='bottom', **hfont3)
    
    plt.ylabel('Cluster size (#IPs)', **hfont)
    plt.xlabel('Autonomous System', **hfont)
    plt.ylim([0, int((float(trueMax) / 10) * 11)])
    plt.xlim([0, len(sortedASes)])
    plt.xticks(ind + 0.5, np.arange(1, len(sortedASes) + 1, 1), **hfont2)
    plt.yticks(np.arange(0, int((float(trueMax) / 10) * 11) + 1, 10), **hfont3)
    
    plt.rc('font', family='serif', size=26)
    plt.legend((rects1[0], rects2[0], rects3[0]), 
               ('Stub AS', 'Transit AS', 'Tier-1 AS'), 
               bbox_to_anchor=(0, 1.02, 1.0, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("LargestClusters.pdf")
    plt.clf()
