/*
 * MiscHop.cpp
 *
 *  Created on: Nov 30, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in MiscHop.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "MiscHop.h"

MiscHop::MiscHop(InetAddress IP)
{
    this->IP = IP;
    this->TTL = 0;
}

MiscHop::MiscHop(unsigned short TTL)
{
    this->IP = InetAddress(0);
    this->TTL = TTL;
}

MiscHop::~MiscHop()
{
}

void MiscHop::connectTo(MiscHop *nextHop)
{
    for(list<MiscHop*>::iterator i = nextHops.begin(); i != nextHops.end(); ++i)
    {
        if((*i) == nextHop)
            return;
    }
    nextHops.push_back(nextHop);
    nextHops.sort(MiscHop::compare);
}

void MiscHop::connectTo(Neighborhood *exit)
{
    for(list<Neighborhood*>::iterator i = exits.begin(); i != exits.end(); ++i)
    {
        if((*i) == exit)
            return;
    }
    exits.push_back(exit);
}

bool MiscHop::compare(MiscHop *s1, MiscHop *s2)
{
    if(s1->getIP() != InetAddress(0) || s2->getIP() != InetAddress(0))
        return s1->getIP() < s2->getIP();
    return s1->getTTL() < s2->getTTL();
}

string MiscHop::toString()
{
    stringstream ss;
    
    for(list<MiscHop*>::iterator it = nextHops.begin(); it != nextHops.end(); ++it)
    {
        MiscHop *cur = (*it);
        if(TTL > 0)
            ss << "0.0.0.0 (TTL=" << TTL << ")";
        else
            ss << IP;
        ss << " -> ";
        if(cur->getIP() == InetAddress(0))
            ss << "0.0.0.0 (TTL=" << cur->getTTL() << ")";
        else
            ss << cur->getIP();
        ss << "\n";
    }
    
    /*
     * Sorting neighborhoods only occurs here, because we assume the print out occurs after a 
     * Pioneer visited the whole graph and therefore numbered the neighborhoods.
     */
    
    exits.sort(Neighborhood::smallerID);
    for(list<Neighborhood*>::iterator it = exits.begin(); it != exits.end(); ++it)
    {
        if(TTL > 0)
            ss << "0.0.0.0 (TTL=" << TTL << ")";
        else
            ss << IP;
        ss << " -> N" << (*it)->getID() << "\n";
    }
    
    return ss.str();
}
