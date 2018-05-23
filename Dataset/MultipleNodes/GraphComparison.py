#! /usr/bin/env python
# -*- coding: utf-8 -*-

# For two given graphs, compare the content of each to evaluate how similar they are.

import os
import sys
import numpy as np

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

def getDensity(matrix):
    dims = matrix.shape
    maxLinks = dims[0] * dims[1]
    
    nbLinks = 0
    for i in range(0, dims[0]):
        for j in range(0, dims[1]):
            nbLinks += matrix[i][j]
    
    return float(nbLinks) / float(maxLinks)

if __name__ == "__main__":

    if len(sys.argv) < 3:
        print("Usage: python GraphComparison.py [first .graph file] [second .graph file]")
        sys.exit()
    
    graph1Path = str(sys.argv[1])
    graph2Path = str(sys.argv[2])
    
    # Checks each file exists
    if not os.path.isfile(graph1Path):
        print("First .graph file does not exist")
        sys.exit()
    if not os.path.isfile(graph2Path):
        print("Second .graph file does not exist")
        sys.exit()
    
    # Gets the content of each file as lines
    with open(graph1Path) as f:
        graph1Lines = f.read().splitlines()
    graph1Vertices = getNeighborhoods(graph1Lines)
    
    with open(graph2Path) as f:
        graph2Lines = f.read().splitlines()
    graph2Vertices = getNeighborhoods(graph2Lines)
    
    commonVertices = set()
    for n in graph1Vertices:
        if n in graph2Vertices:
            commonVertices.add(n)
    
    total1 = (float(len(commonVertices)) / float(len(graph1Vertices))) * 100
    
    print("Common vertices: " + str('%.2f' % total1) + '%')
    
    graph1 = getAdjacencyMatrix(graph1Lines, graph1Vertices)
    graph2 = getAdjacencyMatrix(graph2Lines, graph2Vertices)
    
    density1 = getDensity(graph1)
    density2 = getDensity(graph2)
    
    print("Density: " + str(density1) + ' vs. ' + str(density2))
    
    commonEdges = set() # As strings, to simplify
    
    # Builds a reverse dict. of graph1Vertices (to check if some N_X exists in the second graph)
    reverseDict = dict()
    for n in graph1Vertices:
        reverseDict[graph1Vertices[n]] = n
    
    totalEdges = 0 # Direct/indirect links that can exist in both graphs
    trueTotalEdges = 0 # Direct/indirect/remote links that can exist in both graphs
    totalEdgesFirstGraph = 0 # Total of links that exist in the first graph
    totalIdentical = 0 # Direct/indirect that exist in both graphs
    for v in commonVertices:
        ID1 = int(graph1Vertices[v][1:])
        ID2 = int(graph2Vertices[v][1:])
        for j in range(0, graph1.shape[1]):
            # Counting edges in the first graph
            if graph1[ID1 - 1][j] > 0:
                totalEdgesFirstGraph += 1
            # For each (in)direct link in the first graph
            if graph1[ID1 - 1][j] == 1:
                ID3 = j + 1
                otherLabel = reverseDict["N" + str(ID3)]
                # The next neighborhood in the topology also exists in the second graph
                if otherLabel in graph2Vertices:
                    totalEdges += 1
                    trueTotalEdges += 1
                    ID4 = int(graph2Vertices[otherLabel][1:])
                    # The link present in the first graph also exists in the second graph
                    if graph2[ID2 - 1][ID4 - 1] == 1:
                        commonEdge = "N" + str(ID1) + " -> N" + str(ID3)
                        commonEdge += " <=> N" + str(ID2) + " -> N" + str(ID4)
                        commonEdges.add(commonEdge)
                        # print(commonEdge)
            # If it's a remote link, count it in the total of links that can exist in both graphs
            elif graph1[ID1 - 1][j] == 2:
                ID3 = j + 1
                otherLabel = reverseDict["N" + str(ID3)]
                if otherLabel in graph2Vertices:
                    trueTotalEdges += 1
    
    ratio1 = (float(len(commonEdges)) / float(totalEdges)) * 100
    ratio2 = (float(len(commonEdges)) / float(trueTotalEdges)) * 100
    ratio3 = (float(len(commonEdges)) / float(totalEdgesFirstGraph)) * 100
    print("Total of (in)direct links that can exist in both graphs:\t" + str(totalEdges))
    print("Total of (in)direct links that exist in both graphs:\t\t" + str(len(commonEdges)) + "\t(" + str('%.2f' % ratio1) + "%)")
    print("Total of links (any kind) that can exist in both graphs:\t" + str(trueTotalEdges) + "\t(" + str('%.2f' % ratio2) + "%)")
    print("Total of edges in the first graph:\t\t\t\t" + str(totalEdgesFirstGraph) + "\t(" + str('%.2f' % ratio3) + "%)")
