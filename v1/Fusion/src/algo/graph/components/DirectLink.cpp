/*
 * DirectLink.cpp
 *
 *  Created on: Nov 13, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in DirectLink.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "DirectLink.h"

DirectLink::DirectLink(Neighborhood *tail, 
                       Neighborhood *head, 
                       SubnetSite *medium) : Edge(Edge::DIRECT_LINK, tail, head)
{
    this->medium = medium;
}

DirectLink::~DirectLink()
{
}

string DirectLink::toString()
{
    stringstream ss;
    
    ss << "N" << tail->getID() << " -> N" << head->getID();
    ss << " via " << medium->getInferredNetworkAddressString();
    
    return ss.str();
}

string DirectLink::toStringVerbose()
{
    stringstream ss;
    
    ss << "Neighborhood " << tail->getFullLabel() << " to neighborhood " << head->getFullLabel();
    ss << " via " << medium->getInferredNetworkAddressString() << " (direct link)";
    
    return ss.str();
}
