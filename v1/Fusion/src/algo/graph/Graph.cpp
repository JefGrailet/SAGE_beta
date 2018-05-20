/*
 * Graph.cpp
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Graph.h (see this file to learn further about the goals of such 
 * a class).
 */

#include <string>
using std::string;
#include <utility>
using std::pair;

// 3 next lines are for outputting data in text files
#include <fstream>
#include <sys/stat.h> // For CHMOD edition
using std::ofstream;

#include "Graph.h"

Graph::Graph()
{
    nbNeighborhoods = 0; // To be set later (via Pioneer)
    subnetMap = new list<SubnetMapEntry*>[SIZE_SUBNET_MAP];
}

Graph::~Graph()
{
    // Deletes miscellaneous hops
    for(map<InetAddress, MiscHop*>::iterator it = miscHops.begin(); it != miscHops.end(); ++it)
    {
        delete it->second;
    }
    miscHops.clear();
    
    map<unsigned char, MiscHop*>::iterator it;
    for(it = missingHops.begin(); it != missingHops.end(); ++it)
    {
        delete it->second;
    }
    missingHops.clear();

    // Deletes subnet map
    for(unsigned int i = 0; i < SIZE_SUBNET_MAP; i++)
    {
        list<SubnetMapEntry*> sList = this->subnetMap[i];
        for(list<SubnetMapEntry*>::iterator i = sList.begin(); i != sList.end(); ++i)
        {
            delete (*i);
        }
    }
    delete[] subnetMap;
    
    // Nodes will be deleted via Mariner
    gates.clear();
}

MiscHop* Graph::getMiscHop(InetAddress IP)
{
    map<InetAddress, MiscHop*>::iterator res = miscHops.find(IP);
    if(res != miscHops.end())
        return res->second;
    
    MiscHop *newHop = new MiscHop(IP);
    miscHops.insert(pair<InetAddress, MiscHop*>(IP, newHop));
    return newHop;
}

MiscHop* Graph::getMiscHop(unsigned char TTL)
{
    map<unsigned char, MiscHop*>::iterator res = missingHops.find(TTL);
    if(res != missingHops.end())
        return res->second;
    
    MiscHop *newHop = new MiscHop(TTL);
    missingHops.insert(pair<unsigned char, MiscHop*>(TTL, newHop));
    return newHop;
}

list<MiscHop*> Graph::getMiscHops()
{
    list<MiscHop*> result;
    
    for(map<InetAddress, MiscHop*>::iterator it = miscHops.begin(); it != miscHops.end(); ++it)
        result.push_back(it->second);
    
    map<unsigned char, MiscHop*>::iterator it;
    for(it = missingHops.begin(); it != missingHops.end(); ++it)
        result.push_back(it->second);
    
    result.sort(MiscHop::compare);
    return result;
}

void Graph::mapSubnetsFrom(Neighborhood *n)
{
    list<SubnetSite*> *subnets = n->getSubnets();
    for(list<SubnetSite*>::iterator it = subnets->begin(); it != subnets->end(); ++it)
    {
        SubnetSite *cur = (*it);
        InetAddress needle = cur->getPivot();
        
        SubnetMapEntry *newEntry = new SubnetMapEntry(cur, n);
        unsigned long index = (needle.getULongAddress() >> 12);
        this->subnetMap[index].push_back(newEntry);
    }
}

void Graph::sortMapEntries()
{
    for(unsigned int i = 0; i < SIZE_SUBNET_MAP; i++)
    {
        if(this->subnetMap[i].size() > 1)
            this->subnetMap[i].sort(SubnetMapEntry::compare);
    }
}

SubnetMapEntry *Graph::getSubnetContaining(InetAddress needle)
{
    unsigned long index = (needle.getULongAddress() >> 12);
    list<SubnetMapEntry*> subnetList = this->subnetMap[index];
    
    for(list<SubnetMapEntry*>::iterator i = subnetList.begin(); i != subnetList.end(); ++i)
    {
        SubnetMapEntry *entry = (*i);
        if(entry->subnet->contains(needle))
            return entry;
    }
    return NULL;
}
