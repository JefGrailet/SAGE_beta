/*
 * SubnetMapEntry.h
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * A simple class that basically amounts to a data structure gathering in a single object a subnet 
 * and the neighborhood it belongs to in the current graph (each graph object has its own subnet 
 * map). It is the data unit used in a subnet map (hence the name of the class), a structure that 
 * is similar to the IP dictionnary which operates a time/memory trade-off for fast look-up of a 
 * subnet encompassing a given IP.
 * 
 * A class of the same name also exists in TreeNET since September 2016, but its contents are 
 * slightly different.
 */

#ifndef SUBNETMAPENTRY_H_
#define SUBNETMAPENTRY_H_

// Inclusion with forward declaration
#include "Graph.h"
class Graph;

#include "../structure/SubnetSite.h"

class SubnetMapEntry
{
public:

    SubnetMapEntry(SubnetSite *subnet, Neighborhood *n);
    ~SubnetMapEntry();
    
    SubnetSite *subnet;
    Neighborhood *n;
    
    static bool compare(SubnetMapEntry *sme1, SubnetMapEntry *sme2);
    
};

#endif /* SUBNETMAPENTRY_H_ */
