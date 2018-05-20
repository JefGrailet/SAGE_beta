/*
 * Aggregate.cpp
 *
 *  Created on: Sept 28, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Aggregate.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "Aggregate.h"

bool Aggregate::isTTLListed(unsigned char TTL)
{
    if(TTLs.size() == 0)
        return false;

    for(list<unsigned char>::iterator it = TTLs.begin(); it != TTLs.end(); ++it)
    {
        if((*it) == TTL)
            return true;
    }
    return false;
}

Aggregate::Aggregate(list<SubnetSite*> subnets)
{
    this->subnets = subnets;
    list<SubnetSite*> ssList = this->subnets;
    
    // Registers all the TTLs at which the neighborhood label IP appears
    unsigned short nbAnomalies = ssList.front()->getNeighborhoodLabelAnomalies();
    for(list<SubnetSite*>::iterator it = ssList.begin(); it != ssList.end(); ++it)
    {
        SubnetSite *cur = (*it);
        if(cur == NULL) // Just in case
            continue;
        short offset = (cur->getFinalRouteSize() - 1 - nbAnomalies);
        if(offset < 0) // Just in case
            continue;
        
        unsigned char curTTL = (unsigned char) ((unsigned short) offset + 1);
        if(!this->isTTLListed(curTTL))
        {
            if(TTLs.size() > 0 && curTTL < TTLs.front())
                TTLs.push_front(curTTL);
            else
                TTLs.push_back(curTTL);
        }
    }
    
    junction = false;
}

Aggregate::~Aggregate()
{
    // Subnets/peers will be moved elsewhere and deleted at the very end, so no deletion here
}

string Aggregate::toString()
{
    stringstream ss; // stringstream included indirectly, via SubnetSite.h
    
    ss << "Aggregate for " << this->getLabelString() << ":\n";
    for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
        ss << (*it)->getInferredNetworkAddressString() << "\n";
    ss << "TTLs for " << this->getLabelIP() << ": ";
    for(list<unsigned char>::iterator it = TTLs.begin(); it != TTLs.end(); ++it)
    {
        if(it != TTLs.begin())
            ss << ", ";
        ss << (unsigned short) (*it);
    }

    return ss.str();
}
