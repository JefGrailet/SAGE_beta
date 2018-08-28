/*
 * Cluster.cpp
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Cluster.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "Cluster.h"

Cluster::Cluster(list<Aggregate*> aggregates) : Neighborhood()
{
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
    {
        Aggregate *cur = (*i);
        InetAddress labelIP = cur->getLabelIP();
        labels.push_back(labelIP);
        junctionLabels.push_back(labelIP);
        
        list<SubnetSite*> *ssList = cur->getSubnets();
        list<Peer*> *curPeers = cur->getPeers();
        
        subnets.insert(subnets.end(), ssList->begin(), ssList->end());
        peers.insert(peers.end(), curPeers->begin(), curPeers->end());
    }
    
    labels.sort(InetAddress::smaller);
    junctionLabels.sort(InetAddress::smaller);
    subnets.sort(SubnetSite::compare);
    peers.sort(Peer::compare);
    
    /*
     * N.B.: the list of peers can contain redundant entries at instantiation time. It's 
     * post-processed during one of the late steps of graph building.
     */
}

Cluster::~Cluster()
{
}

void Cluster::addLabel(InetAddress alias)
{
    labels.push_back(alias);
    labels.sort(InetAddress::smaller);
}

list<InetAddress> Cluster::listAllInterfaces()
{
    list<InetAddress> result;
    for(list<InetAddress>::iterator i = labels.begin(); i != labels.end(); ++i)
        result.push_back((*i));
    
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

string Cluster::getFullLabel()
{
    stringstream ss;
    for(list<InetAddress>::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        if(i != labels.begin())
            ss << ", ";
        ss << (*i);
    }
    return ss.str();
}

string Cluster::toString()
{
    stringstream ss; // stringstream included indirectly, via SubnetSite.h
    
    if(ID == 0)
        ss << "Neighborhood - ";
    else
        ss << "N" << ID << " - ";
    
    ss << "Cluster {";
    for(list<InetAddress>::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        if(i != labels.begin())
            ss << ", ";
        ss << (*i);
    }
    ss << "}";
    if(junctionLabels.size() == 1)
        ss << " (only one label IP)";
    ss << ":\n";
    for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
    {
        SubnetSite *cur = (*it);
        
        ss << cur->getInferredNetworkAddressString();
        if(junctionLabels.size() > 1)
            ss << " (neighborhood label: " << cur->getNeighborhoodLabelIP() << ")";
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
