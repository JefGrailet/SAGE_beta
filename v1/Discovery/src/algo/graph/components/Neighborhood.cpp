/*
 * Neighborhood.cpp
 *
 *  Created on: Sept 26, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Neighborhood.h (see this file to learn further about the goals 
 * of such a class).
 */

#include "Neighborhood.h"

Neighborhood::Neighborhood()
{
    this->ID = 0; // Set later
}

Neighborhood::~Neighborhood()
{
    for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
    {
        delete (*it);
    }
    subnets.clear();
    
    for(list<Router*>::iterator it = aliases.begin(); it != aliases.end(); ++it)
    {
        delete (*it);
    }
    aliases.clear();
    
    for(list<Peer*>::iterator it = peers.begin(); it != peers.end(); ++it)
    {
        delete (*it);
    }
    peers.clear();
    
    // Only "out" edges are deleted, because "in" edges will be deleted with the predecessor
    for(list<Edge*>::iterator it = outEdges.begin(); it != outEdges.end(); ++it)
    {
        delete (*it);
    }
    inEdges.clear();
    outEdges.clear();
}

bool Neighborhood::isConnectedTo(Neighborhood *n)
{
    for(list<Edge*>::iterator it = inEdges.begin(); it != inEdges.end(); ++it)
    {
        if((*it)->getTail() == n)
            return true;
    }
    for(list<Edge*>::iterator it = outEdges.begin(); it != outEdges.end(); ++it)
    {
        if((*it)->getHead() == n)
            return true;
    }
    return false;
}

void Neighborhood::getSubnetMetrics(unsigned int *metrics)
{
    // Resets array first
    metrics[0] = 0;
    metrics[1] = 0;
    metrics[2] = 0;
    metrics[3] = 0;
    metrics[4] = 0;
    metrics[5] = 0;

    // Fills it
    for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
    {
        SubnetSite *cur = (*it);
        unsigned short status = cur->getStatus();
        unsigned int capacity = cur->getCapacity();
        if(status == SubnetSite::SHADOW_SUBNET)
        {
            metrics[2]++;
        }
        else
        {
            if(cur->isCredible())
            {
                metrics[3]++;
                metrics[5] += capacity;
            }
            
            if(status == SubnetSite::ACCURATE_SUBNET)
                metrics[0]++;
            else
                metrics[1]++;
        }
        metrics[4] += capacity;
    }
}

unsigned short Neighborhood::getLargestAliasSize()
{
    unsigned short maxSize = 0;
    for(list<Router*>::iterator it = aliases.begin(); it != aliases.end(); ++it)
    {
        Router *cur = (*it);
        unsigned short curSize = cur->getNbInterfaces();
        if(curSize > maxSize)
            maxSize = curSize;
    }
    return maxSize;
}

SubnetSite* Neighborhood::getConnectingSubnet(InetAddress nextHop)
{
    for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
    {
        SubnetSite *cur = (*it);
        if(cur->contains(nextHop))
            return cur;
    }
    return NULL;
}

bool Neighborhood::smallerID(Neighborhood *n1, Neighborhood *n2)
{
    return n1->getID() < n2->getID();
}
