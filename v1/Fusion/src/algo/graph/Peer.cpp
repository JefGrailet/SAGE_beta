/*
 * Peer.cpp
 *
 *  Created on: Oct 12, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Peer.h (see this file to learn further about the goals of such 
 * a class).
 */

#include "Peer.h"

Peer::Peer(unsigned short offset, Router *alias)
{
    this->offset = offset;
    this->concretePeer = NULL;
    
    list<RouterInterface*> *IPs = alias->getInterfacesList();
    for(list<RouterInterface*>::iterator it = IPs->begin(); it != IPs->end(); ++it)
        interfaces.push_back((*it)->ip);
    interfaces.sort(InetAddress::smaller);
}

Peer::Peer(unsigned short offset, InetAddress junction)
{
    this->offset = offset;
    this->concretePeer = NULL;
    
    interfaces.push_back(junction);
}

Peer::~Peer()
{
}

bool Peer::contains(InetAddress interface)
{
    for(list<InetAddress>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        if((*i) == interface)
            return true;
    }
    return false;
}

bool Peer::isSimilarTo(Peer *p)
{
    list<InetAddress> *peerIPs = p->getInterfaces();
    for(list<InetAddress>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        for(list<InetAddress>::iterator j = peerIPs->begin(); j != peerIPs->end(); ++j)
        {
            if((*i) == (*j)) // A single common IP is enough to consider merging peers
                return true;
        }
    }
    return false;
}

bool Peer::equals(Peer *p)
{
    list<InetAddress> *peerIPs = p->getInterfaces();
    if(interfaces.size() == peerIPs->size())
    {
        list<InetAddress>::iterator i = interfaces.begin();
        list<InetAddress>::iterator j = peerIPs->begin();
        while(i != interfaces.end() && (*i) == (*j))
        {
            i++;
            j++;
        }
        if(i == interfaces.end())
            return true;
        return false;
    }
    return false;
}

void Peer::mergeWith(Peer *mergee)
{
    // Adds IPs from mergee not found in this peer
    list<InetAddress> *peerIPs = mergee->getInterfaces();
    for(list<InetAddress>::iterator i = peerIPs->begin(); i != peerIPs->end(); ++i)
    {
        bool found = false;
        for(list<InetAddress>::iterator j = interfaces.begin(); j != interfaces.end(); ++j)
        {
            if((*i) == (*j))
            {
                found = true;
                break;
            }
        }
        
        if(!found)
            interfaces.push_back((*i));
    }
    interfaces.sort(InetAddress::smaller);
}

bool Peer::compare(Peer *p1, Peer *p2)
{
    unsigned short offset1 = p1->getOffset();
    unsigned short offset2 = p2->getOffset();
    list<InetAddress> *IPs1 = p1->getInterfaces();
    list<InetAddress> *IPs2 = p2->getInterfaces();
    if(offset1 < offset2 || (offset1 == offset2 && IPs1->front() < IPs2->front()))
        return true;
    return false;
}

string Peer::toString(bool showOffset)
{
    stringstream ss; // stringstream included indirectly, via Neighborhood then SubnetSite.h
    
    if(offset > 1 && showOffset)
    {
        ss << "[Offset=" << offset << "] ";
    }
    
    for(list<InetAddress>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            ss << ", ";
        ss << (*it);
    }

    return ss.str();
}
