/*
 * Node.cpp
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Node.h (see this file to learn further about the goals of such 
 * a class).
 */

#include "Node.h"

Node::Node(Aggregate *a) : Neighborhood()
{
    list<SubnetSite*> *ssList = a->getSubnets();
    list<Peer*> *aPeers = a->getPeers();
    
    subnets.insert(subnets.end(), ssList->begin(), ssList->end());
    peers.insert(peers.end(), aPeers->begin(), aPeers->end());
    
    this->label = a->getLabelIP();
    this->labelAnomalies = a->getLabelAnomalies();
}

Node::~Node()
{
}

list<InetAddress> Node::listAllInterfaces()
{
    list<InetAddress> result;
    if(labelAnomalies == 0)
        result.push_back(label);
    
    // Lists relevant interfaces for each subnet
    for(list<SubnetSite*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        SubnetSite *cur = (*i);
        unsigned short state = cur->getStatus();
        unsigned char shortestTTL = cur->getShortestTTL();
        if(state == SubnetSite::ACCURATE_SUBNET || state == SubnetSite::ODD_SUBNET)
        {
            list<SubnetSiteNode*> *ssn = cur->getSubnetIPList();
            for(list<SubnetSiteNode*>::iterator j = ssn->begin(); j != ssn->end(); ++j)
            {
                SubnetSiteNode *curSSN = (*j);
                if(curSSN->TTL == shortestTTL)
                    result.push_back(curSSN->ip);
            }
        }
    }
    
    return result;
}

string Node::getFullLabel()
{
    stringstream ss;
    ss << label;
    if(labelAnomalies > 0)
        ss << " [" << labelAnomalies << "]";
    return ss.str();
}

string Node::toString()
{
    stringstream ss; // stringstream included indirectly, via SubnetSite.h
    
    if(ID == 0)
        ss << "Neighborhood - ";
    else
        ss << "N" << ID << " - ";
    
    ss << "Node " << label;
    if(labelAnomalies > 0)
        ss << " (#anomalies = " << labelAnomalies << ")";
    ss << ":\n";
    for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
    {
        SubnetSite *cur = (*it);
        ss << cur->getInferredNetworkAddressString();
        unsigned short status = cur->getStatus();
        if(status == SubnetSite::ACCURATE_SUBNET)
            ss << " - Accurate";
        else if(status == SubnetSite::ODD_SUBNET)
        {
            ss << " - Odd";
            if(cur->isCredible())
                ss << " and credible";
        }
        else
            ss << " - Shadow";
        ss << "\n";
    }
    
    if(inEdges.size() > 0)
    {
        if(inEdges.size() == 1)
        {
            Neighborhood *peer = inEdges.front()->getTail();
            ss << "Peer: ";
            if(peer->getID() != 0)
                ss << "N" << peer->getID() << " - ";
            ss << peer->getFullLabel() << "\n";
        }
        else
        {
            ss << "Peers:\n";
            for(list<Edge*>::iterator it = inEdges.begin(); it != inEdges.end(); ++it)
            {
                Neighborhood *peer = (*it)->getTail();
                if(peer->getID() != 0)
                    ss << "N" << peer->getID() << " - ";
                ss << peer->getFullLabel() << "\n";
            }
        }
    }

    return ss.str();
}
