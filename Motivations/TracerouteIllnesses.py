#! /usr/bin/env python
# -*- coding: utf-8 -*-

# For each dataset of a given date for a given list of ASes, parses the routes and the IPs that 
# appear in it, spots the stretched last hops, anonymous hops, and finally plots as a bar chart, 
# for each AS, the ratio of routes impacted by one of the following issues:
# -route contains anonymous hops, 
# -last hop is anonymous, 
# -last hop is stretched.
# N.B.: by "last hop", one must understand "last hop BEFORE the subnet". Indeed, if you add the 
# destination IP to the route, then the last hop is rather the penultimate hop.

import os
import sys
import numpy as np
from matplotlib import pyplot as plt

if __name__ == "__main__":

    if len(sys.argv) < 4:
        print("Use this command: python TracerouteIllnesses.py [year] [date] [path to AS file]")
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
    parsedASes = [] # To have a list of successfully parsed ASes
    ratiosAnonymous = []
    ratiosEndAnonymous = []
    ratiosEndStretched = []
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
                # Replaces "Anonymous" with 0.0.0.0
                if route[k] == 'Anonymous' or route[k] == 'Missing':
                    route[k] = '0.0.0.0'
                # Removes the potential tag
                elif '[' in route[k]:
                    splitted = route[k].split(" [")
                    route[k] = splitted[0]
            formattedSubnets[j][3] = route
        
        # Makes a census of last hops and stretched last hops.
        lastHops = set()
        baseTTLs = dict()
        stretchedLastHops = set() 
        for j in range(0, len(formattedSubnets)):
            route = formattedSubnets[j][3]
            IP = route[len(route) - 1]
            if IP not in lastHops:
                lastHops.add(IP)
            if IP not in baseTTLs:
                baseTTLs[IP] = len(route)
            elif baseTTLs[IP] < len(route):
                if IP not in stretchedLastHops:
                    stretchedLastHops.add(IP)
        
        # Quantifies ill routes
        nbWithAnonymous = 0
        nbWithEndAnonymous = 0
        nbWithEndStretched = 0
        for j in range(0, len(formattedSubnets)):
            route = formattedSubnets[j][3]
            withAnonymous = False
            withEndAnonymous = False
            for k in range(0, len(route)):
                if route[k] == '0.0.0.0':
                    withAnonymous = True
                    if k == len(route) - 1:
                        withEndAnonymous = True
            if withAnonymous:
                nbWithAnonymous += 1
            if withEndAnonymous:
                nbWithEndAnonymous += 1
            if route[len(route) - 1] in stretchedLastHops:
                nbWithEndStretched += 1
        
        totalOfRoutes = float(len(formattedSubnets))
        ratioAnonymous = float(nbWithAnonymous) / totalOfRoutes * 100
        ratioEndAnonymous = float(nbWithEndAnonymous) / totalOfRoutes * 100
        ratioEndStretched = float(nbWithEndStretched) / totalOfRoutes * 100
        
        print(ASes[i] + ":")
        print("With anonymous hops: " + str(ratioAnonymous) + "%")
        print("Ending with anonymous hop: " + str(ratioEndAnonymous) + "%")
        print("Ending with stretched IP: " + str(ratioEndStretched) + "%")
        print("")
        
        parsedASes.append(ASes[i][2:])
        ratiosAnonymous.append(ratioAnonymous)
        ratiosEndAnonymous.append(ratioEndAnonymous)
        ratiosEndStretched.append(ratioEndStretched)
    
    # Plots in a bar chart (4 bars per AS) the ratios
    ind = np.arange(len(ASes))
    width = 0.3
    
    fig = plt.figure(figsize=(18,12))
    ax = fig.add_subplot(111)
    rects1 = ax.bar(ind + 0.05, ratiosAnonymous, width, color='#333333')
    rects2 = ax.bar(ind + 0.05 + width, ratiosEndAnonymous, width, color='#AAAAAA')
    rects3 = ax.bar(ind + 0.05 + width * 2, ratiosEndStretched, width, color='#EEEEEE')
    ax.autoscale(tight=True)
    ax.yaxis.grid(True)
    
    hfont = {'fontname':'serif',
             'fontweight':'bold',
             'fontsize':36}
    
    hfont2 = {'fontname':'serif',
              'fontsize':28}
    
    hfont3 = {'fontname':'serif',
              'fontsize':28}
    
    plt.ylabel('Ratio (%) of routes', **hfont)
    plt.xlabel('Autonomous System Number (ASN)', **hfont)
    plt.ylim([0,70])
    plt.xlim([0,len(ASes)])
    plt.xticks(ind + 0.5, parsedASes, **hfont2)
    plt.yticks(np.arange(0, 71, 10), **hfont3)
    
    plt.rc('font', family='serif', size=24)
    plt.legend((rects1[0], rects2[0], rects3[0]), 
               ('With anon. hops', 'Anon. last hop', 'Stretched last hop'), 
               bbox_to_anchor=(0, 1.02, 1.0, .102), 
               loc=3,
               ncol=4, 
               mode="expand", 
               borderaxespad=0.)

    plt.savefig("TracerouteIllnesses_" + yearOfMeasurements + "_" + dateOfMeasurements + ".pdf")
    plt.clf()
