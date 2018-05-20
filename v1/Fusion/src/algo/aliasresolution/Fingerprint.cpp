/*
 * Fingerprint.cpp
 *
 *  Created on: Mar 7, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in Fingerprint.h (see this file to learn further about its goals).
 */

#include "Fingerprint.h"

Fingerprint::Fingerprint(IPTableEntry *ip)
{
    this->ipEntry = ip;
    
    this->initialTTL = ip->getEchoInitialTTL();
    this->portUnreachableSrcIP = ip->getPortUnreachableSrcIP();
    this->IPIDCounterType = ip->getIPIDCounterType();
    this->hostName = ip->getHostName();
    this->replyingToTSRequest = ip->repliesToTSRequest();
}

Fingerprint::~Fingerprint()
{
}

bool Fingerprint::compare(Fingerprint &f1, Fingerprint &f2)
{
    if (f1.initialTTL > f2.initialTTL)
    {
        return true;
    }
    else if(f1.initialTTL == f2.initialTTL)
    {
        // Because we want "high" IPs before (put 0.0.0.0's at the bottom of the list)
        if(f1.portUnreachableSrcIP > f2.portUnreachableSrcIP)
        {
            return true;
        }
        else if(f1.portUnreachableSrcIP == f2.portUnreachableSrcIP)
        {
            if(f1.IPIDCounterType > f2.IPIDCounterType)
            {
                return true;
            }
            else if(f1.IPIDCounterType == f2.IPIDCounterType)
            {
                if(!f1.hostName.empty() && f2.hostName.empty())
                {
                    return true;
                }
                else if((f1.hostName.empty() && f2.hostName.empty()) || 
                        (!f1.hostName.empty() && !f2.hostName.empty()))
                {
                    if(!f1.replyingToTSRequest && f2.replyingToTSRequest)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool Fingerprint::compareBis(Fingerprint &f1, Fingerprint &f2)
{
    InetAddress ip1 = (InetAddress) (*(f1.ipEntry));
    InetAddress ip2 = (InetAddress) (*(f2.ipEntry));
    return ip1 < ip2;
}

bool Fingerprint::equals(Fingerprint &f)
{
    // Remark (October 2017): ICMP timestamp request no longer considered for the strict equality.
    if(this->initialTTL == f.initialTTL && 
       this->portUnreachableSrcIP == f.portUnreachableSrcIP && 
       this->IPIDCounterType == f.IPIDCounterType)
    {
        return true;
    }
    return false;
}

bool Fingerprint::toGroupByDefault()
{
    switch(this->IPIDCounterType)
    {
        case IPTableEntry::RANDOM_COUNTER:
        case IPTableEntry::ECHO_COUNTER:
            return true;
        default:
            break;
    }
    return false;
}
