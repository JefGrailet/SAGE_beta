/*
 * IndirectLink.cpp
 *
 *  Created on: Nov 13, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in IndirectLink.h (see this file to learn further about the goals 
 * of such a class).
 */

#include "IndirectLink.h"

IndirectLink::IndirectLink(Neighborhood *tail, 
                           Neighborhood *head, 
                           SubnetMapEntry *medium) : Edge(Edge::INDIRECT_LINK, tail, head)
{
    this->medium = medium;
}

IndirectLink::IndirectLink(Neighborhood *tail, 
                           Neighborhood *head) : Edge(Edge::INDIRECT_LINK, tail, head)
{
    medium = NULL;
}

IndirectLink::~IndirectLink()
{
}

string IndirectLink::toString()
{
    stringstream ss;
    
    ss << "N" << tail->getID() << " -> N" << head->getID();
    if(medium != NULL)
    {
        ss << " via " << medium->subnet->getInferredNetworkAddressString();
        ss << " (from N" << medium->n->getID() << ")";
    }
    else
    {
        ss << " (unknown medium)";
    }
    
    return ss.str();
}

string IndirectLink::toStringVerbose()
{
    stringstream ss;
    
    ss << "Neighborhood " << tail->getFullLabel() << " to neighborhood " << head->getFullLabel();
    if(medium != NULL)
    {
        ss << " via " << medium->subnet->getInferredNetworkAddressString();
        ss << " (from neighborhood " << medium->n->getFullLabel() << "; indirect link)";
    }
    else
    {
        ss << " via unknown medium";
    }
    
    return ss.str();
}
