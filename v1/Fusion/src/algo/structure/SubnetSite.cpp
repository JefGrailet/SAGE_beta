/*
 * SubnetSite.cpp
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Implements the class defined in SubnetSite.h (see this file for more details).
 */

#include <string> // For compare() in a static comparison method
#include <math.h> // For pow() function

#include "SubnetSite.h"

#include "../../common/inet/NetworkAddress.h"

SubnetSite::SubnetSite():
inferredSubnetBaseIP(0), 
inferredSubnetPrefix(255), 
status(0), 
contrapivot(0), 
TTL1(0), 
TTL2(0), 
routeTarget(0), 
routeSize(0), 
route(NULL), 
penultimateShift(0), 
neighborhoodLabelIP(0), 
neighborhoodLabelAnomalies(0)
{

}

SubnetSite::~SubnetSite()
{
    clearIPlist();
    if(route != NULL)
        delete[] route;
}

void SubnetSite::clearIPlist()
{
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL)
            delete (*i);
    }
    IPlist.clear();
}

string SubnetSite::getInferredNetworkAddressString()
{
    if(inferredSubnetPrefix > 32)
    {
        return string("");
    }
    else if(inferredSubnetPrefix == 32)
    {
        return inferredSubnetBaseIP.getHumanReadableRepresentation() + "/32";
    }
    else
    {
        NetworkAddress na(inferredSubnetBaseIP, inferredSubnetPrefix);
        return na.getHumanReadableRepresentation();
    }
}

/*
 * ------------------------------
 * Methods for refinement/tracing
 * ------------------------------
 */

unsigned short SubnetSite::mergeNodesWith(SubnetSite *ss)
{
    list<SubnetSiteNode*> *otherIPs = ss->getSubnetIPList();
    unsigned short newInterfaces = 0;
    for(list<SubnetSiteNode*>::iterator i = otherIPs->begin(); i != otherIPs->end(); ++i)
    {
        SubnetSiteNode *cur = (*i);
        
        // Checks if the current node is already in IPlist
        bool found = false;
        for(list<SubnetSiteNode*>::iterator j = IPlist.begin(); j != IPlist.end(); ++j)
        {
            SubnetSiteNode *curBis = (*j);
            if(cur->ip == curBis->ip)
            {
                found = true;
                break;
            }
        }
        
        if(!found)
        {
            newInterfaces++;
            IPlist.push_back(cur);
            otherIPs->erase(i--); // If not, deleting ss will cause memory issues with this
        }
    }
    IPlist.sort(SubnetSiteNode::smaller);
    return newInterfaces;
}

void SubnetSite::completeRefinedData()
{
    // Goes through the list of IPs to find shortest/greatest TTL
    unsigned char shortestTTL = 0, greatestTTL = 0;
    InetAddress contrapivotCandidate(0);
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        SubnetSiteNode *cur = (*i);

        if(greatestTTL == 0 || cur->TTL > greatestTTL)
            greatestTTL = cur->TTL;
        
        if(shortestTTL == 0 || cur->TTL < shortestTTL)
        {
            contrapivotCandidate = cur->ip;
            shortestTTL = cur->TTL;
        }
    }

    this->TTL1 = shortestTTL;
    this->TTL2 = greatestTTL;

    // No more condition "shortestTTL == greatestTTL - 1" because of outliers
    this->contrapivot = contrapivotCandidate;
    
    /*
     * We also re-compute the subnet status (mainly for ACCURATE/ODD distinction).
     */
    
    if(shortestTTL == greatestTTL)
    {
        this->status = SubnetSite::SHADOW_SUBNET;
    }
    // 2 TTLs with a difference of 1: subnet might be accurate
    else if((unsigned short) shortestTTL == (((unsigned short) greatestTTL) - 1))
    {
        unsigned short nbContrapivot = 0;
        InetAddress smallestContrapivot = contrapivotCandidate;
        for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
        {
            SubnetSiteNode *cur = (*i);
            if(cur != NULL && cur->TTL == shortestTTL)
            {
                nbContrapivot++;
                if(cur->ip < smallestContrapivot)
                    smallestContrapivot = cur->ip;
            }
        }
        
        if(nbContrapivot == 1)
            this->status = SubnetSite::ACCURATE_SUBNET;
        else
        {
            this->status = SubnetSite::ODD_SUBNET;
            this->contrapivot = smallestContrapivot;
        }
    }
    // Any other case: subnet is odd
    else
    {
        this->status = SubnetSite::ODD_SUBNET;
    }
}

InetAddress SubnetSite::getPivot()
{
    // Undefined subnet: aborts
    if(this->status == SubnetSite::UNDEFINED_SUBNET)
    {
        return InetAddress(0);
    }
    
    // Shadow subnet: gets the IP of the first "visible" address
    if(this->status == SubnetSite::SHADOW_SUBNET)
    {
        return IPlist.front()->ip;
    }
    
    unsigned short shortestTTL = this->TTL1;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (*i)->TTL == shortestTTL + 1)
        {
            return (*i)->ip;
        }
    }
    
    // IP 0 if no result (unlikely)
    return InetAddress(0);
}

bool SubnetSite::hasCompleteRoute()
{
    for(unsigned short i = 0; i < routeSize; ++i)
        if(route[i].ip == InetAddress(0))
            return false;
    return true;
}

bool SubnetSite::hasIncompleteRoute()
{
    for(unsigned short i = 0; i < routeSize; ++i)
        if(route[i].ip == InetAddress(0))
            return true;
    return false;
}

unsigned short SubnetSite::countMissingHops()
{
    unsigned short res = 0;
    for(unsigned short i = 0; i < routeSize; ++i)
        if(route[i].ip == InetAddress(0))
            res++;
    return res;
}

bool SubnetSite::compare(SubnetSite *ss1, SubnetSite *ss2)
{
    unsigned long int prefix1 = (ss1->getInferredSubnetBaseIP()).getULongAddress();
    unsigned long int prefix2 = (ss2->getInferredSubnetBaseIP()).getULongAddress();
    
    if (prefix1 < prefix2)
        return true;
    return false;
}

bool SubnetSite::compareAlt(SubnetSite *ss1, SubnetSite *ss2)
{
    unsigned long int initPrefix1 = (ss1->getInferredSubnetBaseIP()).getULongAddress();
    unsigned long int prefix1 = initPrefix1 >> 12;
    prefix1 = prefix1 << 12;
    
    unsigned long int initPrefix2 = (ss2->getInferredSubnetBaseIP()).getULongAddress();
    unsigned long int prefix2 = initPrefix2 >> 12;
    prefix2 = prefix2 << 12;
    
    if(prefix1 < prefix2)
    {
        return true;
    }
    else if(prefix1 == prefix2)
    {
        unsigned long int prefix1Bis = (ss1->getInferredSubnetBaseIP()).getULongAddress();
        unsigned long int prefix2Bis = (ss2->getInferredSubnetBaseIP()).getULongAddress();
        
        if(prefix1Bis > prefix2Bis)
            return true;
        return false;
    }
    return false;
}

bool SubnetSite::compareRoutes(SubnetSite *ss1, SubnetSite *ss2)
{
    unsigned short size1 = ss1->getRouteSize();
    unsigned short size2 = ss2->getRouteSize();

    if(size1 == 0 && size2 == 0)
        return compare(ss1, ss2);
    
    if (size1 < size2)
        return true;
    return false;
}

bool SubnetSite::encompasses(SubnetSite *ss2)
{
    if(this->getInferredSubnetPrefixLength() == 32)
        return false;

    NetworkAddress na = this->getInferredNetworkAddress();
    InetAddress lowerBorder = na.getLowerBorderAddress();
    InetAddress upperBorder = na.getUpperBorderAddress();
    
    if(ss2->getInferredSubnetPrefixLength() == 32)
    {
        InetAddress pivotIP = ss2->getInferredSubnetBaseIP();
        if(lowerBorder <= pivotIP && upperBorder >= pivotIP)
        {
            return true;
        }
        return false;
    }
    
    NetworkAddress na2 = ss2->getInferredNetworkAddress();
    InetAddress lowerBorder2 = na2.getLowerBorderAddress();
    InetAddress upperBorder2 = na2.getUpperBorderAddress();
    
    if(lowerBorder <= lowerBorder2 && upperBorder >= upperBorder2)
        return true;
    return false;
}

/*
 * -------------
 * Miscellaneous
 * -------------
 */

bool SubnetSite::contains(InetAddress i)
{
    if(this->inferredSubnetPrefix >= 32)
        return false;

    NetworkAddress na = this->getInferredNetworkAddress();
    InetAddress lowerBorder = na.getLowerBorderAddress();
    InetAddress upperBorder = na.getUpperBorderAddress();
    
    if(i >= lowerBorder && i <= upperBorder)
        return true;
    return false;
}

bool SubnetSite::hasPivot(InetAddress i)
{
    if(this->inferredSubnetPrefix >= 32)
        return false;

    NetworkAddress na = this->getInferredNetworkAddress();
    InetAddress lowerBorder = na.getLowerBorderAddress();
    InetAddress upperBorder = na.getUpperBorderAddress();
    
    if(i >= lowerBorder && i <= upperBorder)
    {
        // No contra-pivot IP: return true anyway
        if(this->TTL1 == this->TTL2)
            return true;
    
        // Checking contra-pivot IPs
        unsigned char contrapivotTTL = this->TTL1;
        for(list<SubnetSiteNode*>::iterator j = IPlist.begin(); j != IPlist.end(); ++j)
            if((*j)->TTL == contrapivotTTL && (*j)->ip == i)
                return false;
        return true;
    }
    return false;
}

SubnetSiteNode* SubnetSite::getNode(InetAddress li)
{
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (*i)->ip == li)
            return (*i);
    }
    return NULL;
}

string SubnetSite::toString()
{
    stringstream ss;
    
    if(status == SubnetSite::ACCURATE_SUBNET || status == SubnetSite::SHADOW_SUBNET || status == SubnetSite::ODD_SUBNET)
    {
        ss << getInferredNetworkAddressString() << "\n";
        if(status == SubnetSite::ACCURATE_SUBNET)
            ss << "ACCURATE\n";
        else if(status == SubnetSite::SHADOW_SUBNET)
            ss << "SHADOW\n";
        else
            ss << "ODD\n";
        
        // Writes live interfaces
        IPlist.sort(SubnetSiteNode::smaller); // Sorts the interfaces
        bool guardian = false;
        InetAddress previous(0);
        for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
        {
            // Security (for PlanetLab version)
            if((*i) == NULL)
                continue;
        
            // Avoids possible duplicates
            if((*i)->ip == previous)
                continue;
            
            if(guardian)
                ss << ", ";
            else
                guardian = true;
        
            ss << (*i)->ip << " - " << (unsigned short) (*i)->TTL;
            
            previous = (*i)->ip;
        }
        ss << "\n";
        
        // Writes (observed) route
        if(routeSize > 0 && route != NULL)
        {
            for(unsigned int i = 0; i < routeSize; i++)
            {
                if(i > 0)
                    ss << ", ";
                
                unsigned short curState = route[i].state;
                if(route[i].ip != InetAddress(0))
                {
                    ss << route[i].ip;
                    if(curState == RouteInterface::REPAIRED_1)
                        ss << " [Repaired-1]";
                    else if(curState == RouteInterface::REPAIRED_2)
                        ss << " [Repaired-2]";
                    else if(curState == RouteInterface::LIMITED)
                        ss << " [Limited]";
                }
                else
                {
                    if(curState == RouteInterface::ANONYMOUS)
                        ss << "Anonymous";
                    else if(curState == RouteInterface::MISSING)
                        ss << "Missing";
                    else
                        ss << "Skipped";
                }
            }
            ss << "\n";
        }
        else
        {
            ss << "No route\n";
        }
    }
    
    return ss.str();
}

bool SubnetSite::isCredible()
{
    bool credibility = false;
    
    unsigned short baseTTL = (unsigned short) this->TTL1;
    unsigned short diffTTL = (unsigned short) this->TTL2 - baseTTL;
    
    if(diffTTL == 0)
        return false;
    
    unsigned int *interfacesByTTL = new unsigned int[diffTTL + 1];
    for (unsigned short i = 0; i < (diffTTL + 1); i++)
        interfacesByTTL[i] = 0;
    
    unsigned int totalInterfaces = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        // Just in case
        if((*i) == NULL)
            continue;
    
        unsigned short offset = (unsigned short) (*i)->TTL - baseTTL;
        if(offset <= diffTTL)
        {
            interfacesByTTL[offset]++;
            totalInterfaces++;
        }
    }
    
    if(diffTTL == 1)
    {
        if(interfacesByTTL[0] == 1)
            credibility = true;
        else
        {
            double ratioContrapivots = (double) interfacesByTTL[0] / (double) totalInterfaces;
            double ratioPivots = (double) interfacesByTTL[1] / (double) totalInterfaces;
            
            if(ratioPivots > ratioContrapivots && (ratioPivots - ratioContrapivots) > 0.25)
            {
                credibility = true;
            }
        }
    }
    else
    {
        double ratioContrapivots = (double) interfacesByTTL[0] / (double) totalInterfaces;
        double ratioPivots = (double) interfacesByTTL[1] / (double) totalInterfaces;
        
        if(ratioPivots >= 0.7 && ratioContrapivots < 0.1)
        {
            credibility = true;
        }
    }
    
    delete[] interfacesByTTL;
    
    return credibility;
}

unsigned int SubnetSite::getCapacity()
{
    double power = 32 - (double) ((unsigned short) inferredSubnetPrefix);
    
    if(power < 0)
        return 0;
    else if(power == 0)
        return 1;
    
    return (unsigned int) pow(2, power);
}

/*
 * -----------------------
 * Graph preparation phase
 * -----------------------
 */

void SubnetSite::computeNeighborhoodLabel()
{
    if(routeSize == 0 || route == NULL)
        return;
    
    // Skipping anonymous hops and IPs belonging to the subnet itself (beside contra-pivot)
    InetAddress lastHop = route[routeSize - 1].ip;
    unsigned short nbAnomalies = 0;
    while((lastHop == InetAddress(0) || this->hasPivot(lastHop)) && (routeSize - 1 - nbAnomalies) >= 0)
    {
        nbAnomalies++;
        lastHop = route[routeSize - 1 - nbAnomalies].ip;
    }
    
    // Increases a bit more nbAnomalies if there is a cycle
    if(routeSize - 1 - nbAnomalies > 0)
    {
        while(route[routeSize - 1 - nbAnomalies].ip == lastHop)
            nbAnomalies++;
        
        if(route[routeSize - 1 - nbAnomalies].ip != lastHop)
            nbAnomalies--;
    }
    
    /*
     * If nbAnomalies == 1 and the penultimate IP belongs to the subnet while being a pivot, we 
     * "shift" the penultimate hop to mitigate the "fake last hop issue" (see comment at the end 
     * of SubnetSite.h).
     */
    
    if(nbAnomalies == 1)
    {
        InetAddress penultimate = route[routeSize - 1].ip;
        if(penultimate != InetAddress(0) && this->hasPivot(penultimate))
        {
            penultimateShift = 1;
            nbAnomalies = 0;
        }
    }
    
    neighborhoodLabelIP = lastHop;
    neighborhoodLabelAnomalies = nbAnomalies;
}

string SubnetSite::getNeighborhoodLabelString()
{
    stringstream ss;
    ss << "[" << neighborhoodLabelIP;
    if(neighborhoodLabelAnomalies > 0)
        ss << ", " << neighborhoodLabelAnomalies;
    ss << "]";
    return ss.str();
}
