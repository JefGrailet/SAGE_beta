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
spCost(0), 
siCost(0), 
mergeAmount(0), 
targetIPaddress(0), 
pivotIPaddress(0), 
prevSiteIPaddress(0), 
prevSiteIPaddressDistance(0), 
inferredSubnetPrefix(255), 
alternativeSubnetPrefix(255), 
contraPivotNode(0), 
refinementStatus(0), 
refinementContrapivot(0), 
refinementTTL1(0), 
refinementTTL2(0), 
newSubnet(false), 
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

/*
 * ---------------------
 * Methods for inference
 * ---------------------
 */

int SubnetSite::getSize(int filterin)
{
    int size = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            size++;
        }
    }

    return size;
}

unsigned char SubnetSite::getMinimumPrefixLength(int filterin)
{
    unsigned char minPrefix = 255; // Maximum possible value
    for(list<SubnetSiteNode *>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            if((*i)->prefix < minPrefix)
            {
                minPrefix = (*i)->prefix;
            }
        }
    }

    return minPrefix;
}

void SubnetSite::markSubnetOvergrowthElements(unsigned char barrierPrefix, int filterin)
{
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            if((*i)->prefix < barrierPrefix)
            {
                (*i)->nodeStatus = SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_SUBNET_OVERGROWTH;
            }
        }
    }
}

void SubnetSite::markSubnetBoundaryIncompatibileElements(unsigned char basePrefix, int filterin)
{
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            if((*i)->prefix < basePrefix)
            {
                (*i)->nodeStatus = SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_BOUNDARY_ADDRESS_INCOMPATIBILITY;
            }
        }
    }
}

bool SubnetSite::contains(InetAddress ip, int filterin)
{
    bool result = false;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            if((*i)->ip == ip)
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

void SubnetSite::adjustRemoteSubnet()
{
    /**
     * Now we have the complete list of subnet elements as well as the aliases (if possible). 
     * Before storing the results to the database we must apply the following rules to preserve 
     * the consistency of the subnet:
     * 1) The lower-border-address and upper-border-address cannot be assigned to a node unless 
     *    it is a /31 subnet
     * 2) If either of upper or lower border address is assigned to a node of subnet /x, x <= 30 
     *    then there are two cases:
     *       i)  The subnet is actually a /x-1 or less subnet but we could not discover it completely;
     *       ii) The subnet is /x+1 or greater subnet but we misconcluded and merged it.
     *
     * As an action, as long as the subnet contains a border address, we divide it into half 
     * until a /31 address.
     */

    /**
     * Normalize the site subnet, that is, remove misinferred items in the subnet and set a 
     * prefixLength for the subnet. Note that the minimum subnetPrefix does not always reflect the 
     * subnetPrefix because if we have a subnet with IPs ending with binary {00,01,10}, this 
     * cannot be a /30 subnet even though the minimum site node is discovered while searching for 
     * /30 subnet. This anomaly raises only if we have /30 as minimum site node prefix and the 
     * reason is different pattern caused by /31.
     */

    /**
     * Moreover, if there are maxSubnetSize IP addresses of subnet has been discovered check if 
     * one has distance d-1 (contra-pivot, i.e., alias) while others have distance d. In case all 
     * of them has distance d than expand the subnet size to cover the contra pivot.
     */

    unsigned char subnetPrefix = this->getMinimumPrefixLength(SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);

    // Marks boundary address incompatible nodes
    NetworkAddress tmpSubnet(InetAddress(0), 31); // Just to temporarily initialize
    bool hasLowerBorder;
    bool hasUpperBorder;
    while(subnetPrefix < 31)
    {
        tmpSubnet = NetworkAddress(this->pivotIPaddress, subnetPrefix);
        hasLowerBorder = this->contains(tmpSubnet.getLowerBorderAddress(), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);
        hasUpperBorder = this->contains(tmpSubnet.getUpperBorderAddress(), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);

        if(hasLowerBorder || hasUpperBorder)
        {
            this->markSubnetBoundaryIncompatibileElements((subnetPrefix + 1), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);
            // Marks all subnets having prefix less than subnetPrefix
        }
        else
        {
            break;
        }
        subnetPrefix++;
    }

    /**
     * Sets inferred and alternative prefix lengths. If there is no contrapivot, just use the 
     * minimum of inside boundaries and boundary incompatible for alternative prefix and inside 
     * boundaries for inferred subnet prefix.
     */
    
    inferredSubnetPrefix = 255;
    alternativeSubnetPrefix = 255;

    contraPivotNode = 0;

    int filterin = (SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES | 
                    SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_BOUNDARY_ADDRESS_INCOMPATIBILITY);
    
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (((*i)->nodeStatus) & filterin) > 0)
        {
            // Either nodes inside subnet boundaries or nodes marked for removal (boundary incompatibility) update alternativeSubnetPrefix
            if((*i)->prefix < alternativeSubnetPrefix)
            {
                alternativeSubnetPrefix = (*i)->prefix;
            }

            // Only nodes marked inside subnet boundaries update inferredSubnetPrefix
            if((*i)->nodeStatus == SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES)
            {
                if((*i)->prefix < inferredSubnetPrefix)
                {
                    inferredSubnetPrefix = (*i)->prefix;
                }
            }

            if((*i)->aliasStatus != SubnetSiteNode::UNKNOWN_ALIAS_SX)
            {
                contraPivotNode = (*i);
            }
        }
    }

    /**
     * If there is a contrapivot, extends the inferred prefix so that it covers the contrapivot
     * and makes the alternative just consisting of nodes marked inside subnet boundaries.
     */
    
    if(contraPivotNode != 0 && contraPivotNode->prefix < inferredSubnetPrefix)
    {
        alternativeSubnetPrefix = inferredSubnetPrefix;
        inferredSubnetPrefix = contraPivotNode->prefix;
    }

}

void SubnetSite::adjustLocalAreaSubnet(NetworkAddress &localAreaNetwork)
{
    this->inferredSubnetPrefix = localAreaNetwork.getPrefixLength();
}


void SubnetSite::adjustRemoteSubnet2(bool useLowerBorderAsWell)
{
    // See beginning of adjustRemoteSubnet() for comments

    bool exists[33];
    bool hasOverGrowth[33];
    bool hasLowerBorder[33];
    bool hasUpperBorder[33];
    for (int i = 0; i <= 32; ++i) 
    {
        exists[i] = false;
        hasOverGrowth[i] = false;
        hasLowerBorder[i] = false;
        hasUpperBorder[i] = false;
    }

    contraPivotNode = 0; // Contrapivot is meaningful only if it is covered by inferred prefix

    for(list<SubnetSiteNode *>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) == NULL)
            continue;
    
        exists[(int) ((*i)->prefix)] = true;
        if((*i)->nodeStatus == SubnetSiteNode::MARKED_FOR_REMOVAL_DUE_TO_SUBNET_OVERGROWTH)
        {
            hasOverGrowth[(int) ((*i)->prefix)] = true;
        }
        else
        {
            if((*i)->aliasStatus != SubnetSiteNode::UNKNOWN_ALIAS_SX)
            {
                contraPivotNode = (*i);
            }
        }
    }

    NetworkAddress tmpSubnet(InetAddress(0), 31); // Just to temporarily initialize
    bool foundLowerBorder;
    bool foundUpperBorder;
    unsigned char subnetPrefix = 30;
    
    //31 and /32 are set to hasLowerBorder false and hasUpperBorder false by default
    while(subnetPrefix > 0)
    {
        if(exists[(int) subnetPrefix] && !hasOverGrowth[(int) subnetPrefix])
        {
            tmpSubnet = NetworkAddress(this->pivotIPaddress, subnetPrefix);
            foundLowerBorder = this->contains(tmpSubnet.getLowerBorderAddress(), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);
            foundUpperBorder = this->contains(tmpSubnet.getUpperBorderAddress(), SubnetSiteNode::MARKED_AS_INSIDE_SUBNET_BOUNDARIES);

            if(foundLowerBorder)
            {
                hasLowerBorder[(int) subnetPrefix] = true;
            }

            if(foundUpperBorder)
            {
                hasUpperBorder[(int) subnetPrefix] = true;
            }
        }
        subnetPrefix--;
    }

    bool hasBorder[33];

    if(useLowerBorderAsWell)
    {
        for (int i = 0; i <= 32; ++i)
        {
            hasBorder[i] = hasUpperBorder[i];
        }
    }
    else
    {
        for (int i = 0; i <= 32; ++i)
        {
            hasBorder[i] = (hasUpperBorder[i] || hasLowerBorder[i]);
        }
    }

    /**
     * inferredSubnetPrefix is the minimum prefix which does not cross the overgrowth nodes and 
     * does not have a border.
     */
    
    inferredSubnetPrefix = 255;
    for(int i = 0; i <= 32; ++i) 
    {
        if(exists[i] && !hasOverGrowth[i] && !hasBorder[i])
        {
            inferredSubnetPrefix = (unsigned char) i;
            break;
        }
    }

    /**
     * alternativeSubnetPrefix is the minimum prefix which does not cross the overgrowth nodes 
     * but covers all discovered IP addresses.
     */
    
    alternativeSubnetPrefix = 255;
    int minExistingNonOvergrowthPrefix = 0;
    for(int i = 0; i <= 32; ++i)
    {
        if(exists[i] && !hasOverGrowth[i])
        {
            minExistingNonOvergrowthPrefix = (unsigned char) i;
            break;
        }
    }

    /**
     * In case alternativeSubnetPrefix contains border IP address just expand it by one more 
     * level (i.e., decrease prefix length) regardless of checking if there is a discovered IP 
     * address at the new level.
     */
    
    if(!hasBorder[minExistingNonOvergrowthPrefix])
    {
        alternativeSubnetPrefix = minExistingNonOvergrowthPrefix;
    }
    else
    {
        if(!hasOverGrowth[minExistingNonOvergrowthPrefix - 1])
        {
            alternativeSubnetPrefix = minExistingNonOvergrowthPrefix - 1;
        }
    }

    /**
     * If there exists a contrapivot node but inferredSubnetPrefix does not cover it, expand 
     * inferredSubnetPrefix by one more level (i.e., decrease prefix length) so that it covers 
     * the contrapivot without any border addresses in case there are borders continue expanding 
     * it until getting rid of all borders.
     */
    
    if(inferredSubnetPrefix != 255) // Only if inferred prefix is not set
    {
        unsigned char tmpPrefix;
        if(contraPivotNode != 0)
        {
            // Inferred prefix does not cover the contrapivot
            if(!hasOverGrowth[(int)contraPivotNode->prefix] && contraPivotNode->prefix < inferredSubnetPrefix)
            {
                tmpPrefix = inferredSubnetPrefix;
                for(int i = (int) contraPivotNode->prefix; i >= 0; i--)
                {
                    if(hasOverGrowth[i])
                    {
                        break;
                    }
                    else
                    {
                        if(!hasBorder[i])
                        {
                            tmpPrefix = (unsigned char) i;
                            i--;
                            
                            /**
                             * If there are larger existing subnets without having overgrowth and 
                             * upper borders, cover them.
                             */
                            
                            while(exists[i] && !hasOverGrowth[i] && !hasBorder[i])
                            {
                                tmpPrefix = (unsigned char) i;
                                i--;
                            }
                            break;
                        }
                    }
                }

                /**
                 * Switch alternative prefix with inferred prefix so that alternative is the most 
                 * compatible prefix length.
                 */
                
                if(tmpPrefix != inferredSubnetPrefix)
                {
                    alternativeSubnetPrefix = inferredSubnetPrefix;
                    inferredSubnetPrefix = tmpPrefix;
                }
            }
        }
        else
        {
            /**
             * If there is no contrapivot at all, it is better to assume that there must have 
             * been one but we did not discover it, hence use alternativeSubnetPrefix which 
             * encompasses all IP addresses as alternativeSubnetPrefix by switching them.
             */
            
            tmpPrefix = inferredSubnetPrefix;
            inferredSubnetPrefix = alternativeSubnetPrefix;
            alternativeSubnetPrefix = tmpPrefix;
        }
    }
}

bool SubnetSite::hasAlternativeSubnet()
{
    return ((alternativeSubnetPrefix != 255) && (alternativeSubnetPrefix != inferredSubnetPrefix));
}

int SubnetSite::getInferredSubnetSize()
{
    int size = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (*i)->prefix >= inferredSubnetPrefix)
        {
            size++;
        }
    }

    return size;
}

int SubnetSite::getAlternativeSubnetSize()
{
    int size = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (*i)->prefix >= alternativeSubnetPrefix)
        {
            size++;
        }
    }

    return size;
}

InetAddress SubnetSite::getInferredSubnetContraPivotAddress()
{
    InetAddress contraPivot(0);
    if(this->contraPivotNode != 0 && (this->contraPivotNode->prefix >= inferredSubnetPrefix))
    {
        contraPivot = this->contraPivotNode->ip;
    }
    return contraPivot;

}
InetAddress SubnetSite::getAlternativeSubnetContraPivotAddress()
{
    InetAddress contraPivot(0);
    if(this->contraPivotNode != 0 && (this->contraPivotNode->prefix >= alternativeSubnetPrefix))
    {
        contraPivot = this->contraPivotNode->ip;
    }
    return contraPivot;
}

string SubnetSite::getInferredNetworkAddressString()
{
    if(inferredSubnetPrefix > 32)
    {
        return string("");
    }
    else if(inferredSubnetPrefix == 32)
    {
        return (*(pivotIPaddress.getHumanReadableRepresentation())) + "/32";
    }
    else
    {
        NetworkAddress na(pivotIPaddress, inferredSubnetPrefix);
        return (*(na.getHumanReadableRepresentation()));
    }
}

string SubnetSite::getAlternativeNetworkAddressString()
{
    if(alternativeSubnetPrefix > 32)
    {
        return string("");
    }
    else if(alternativeSubnetPrefix == 32)
    {
        return (*(pivotIPaddress.getHumanReadableRepresentation())) + "/32";
    }
    else
    {
        NetworkAddress na(pivotIPaddress, alternativeSubnetPrefix);
        return (*(na.getHumanReadableRepresentation()));
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
    if(newInterfaces > 0)
        mergeAmount += ss->getMergeAmount() + 1;
    return newInterfaces;
}

void SubnetSite::prepareTTLsForRefinement()
{
    unsigned char shortestTTL = 0, greatestTTL = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        SubnetSiteNode *cur = (*i);
        
        /*
         * Ignores nodes for which the registered prefix is smaller than the prefix of this site.
         * This condition is present in the original ExploreNET.
         */
        
        if(cur != NULL && cur->prefix >= this->inferredSubnetPrefix)
        {
            if(greatestTTL == 0 || cur->TTL > greatestTTL)
                greatestTTL = cur->TTL;
            
            if(shortestTTL == 0 || cur->TTL < shortestTTL)
                shortestTTL = cur->TTL;
        }
    }
    
    this->refinementTTL1 = shortestTTL;
    this->refinementTTL2 = greatestTTL;
}

void SubnetSite::prepareForRefinement()
{
    // If there is only one IP, cannot tell if it is a (contra-)pivot
    if(IPlist.size() == 1)
    {
        this->refinementStatus = SubnetSite::INCOMPLETE_SUBNET;
        this->refinementTTL1 = IPlist.front()->TTL;
        this->refinementTTL2 = IPlist.front()->TTL;
        return;
    }
    
    this->prepareTTLsForRefinement();
    unsigned char shortestTTL = this->refinementTTL1;
    unsigned char greatestTTL = this->refinementTTL2;
    
    // Same TTL everywhere: subnet is incomplete for sure
    if(shortestTTL == greatestTTL)
    {
        this->refinementStatus = SubnetSite::INCOMPLETE_SUBNET;
    }
    // 2 TTLs with a difference of 1: subnet might be accurate
    else if((unsigned short) shortestTTL == (((unsigned short) greatestTTL) - 1))
    {
        // Checks a list a second time to confirm there is only one smallestTTL (= contra-pivot)
        bool foundContrapivot = false;
        InetAddress tempContrapivot(0);
        for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
        {
            SubnetSiteNode *cur = (*i);
            
            if(cur != NULL && cur->TTL == shortestTTL)
            {
                if(!foundContrapivot)
                {
                    tempContrapivot = cur->ip;
                    foundContrapivot = true;
                }
                else
                {
                    this->refinementStatus = SubnetSite::ODD_SUBNET;
                    this->refinementContrapivot = tempContrapivot; // Still needed for tree labels
                    return;
                }
            }
        }
        this->refinementStatus = SubnetSite::ACCURATE_SUBNET;
        this->refinementContrapivot = tempContrapivot;
    }
    // Any other case: subnet is odd
    else
    {
        this->refinementStatus = SubnetSite::ODD_SUBNET;
        
        // Still needs to set the refinementContrapivot field
        for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
        {
            SubnetSiteNode *cur = (*i);
            if(cur->TTL == shortestTTL)
            {
                this->refinementContrapivot = cur->ip;
                break;
            }
        }
    }
}

void SubnetSite::recomputeRefinementStatus()
{
    // Not ACCURATE: aborts (we only double check status for ACCURATE subnets)
    if(this->refinementStatus != SubnetSite::ACCURATE_SUBNET)
    {
        return;
    }
    
    // Double-checks shortest/greatest TTLs
    this->prepareTTLsForRefinement();
    unsigned short shortestTTL = this->refinementTTL1;
    unsigned short greatestTTL = this->refinementTTL2;

    // 2 TTLs with a difference of 1: subnet might be accurate
    if((unsigned short) shortestTTL == (((unsigned short) greatestTTL) - 1))
    {
        unsigned short nbContrapivot = 0;
        InetAddress smallestContrapivot = this->refinementContrapivot;
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
            this->refinementStatus = SubnetSite::ACCURATE_SUBNET;
        else
        {
            this->refinementStatus = SubnetSite::ODD_SUBNET;
            this->refinementContrapivot = smallestContrapivot;
        }
    }
    // Any other case: subnet is odd
    else
    {
        this->refinementStatus = SubnetSite::ODD_SUBNET;
        
        // Ensures the refinementContrapivot field is set
        for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
        {
            SubnetSiteNode *cur = (*i);
            if(cur->TTL == shortestTTL)
            {
                this->refinementContrapivot = cur->ip;
                break;
            }
        }
    }
}

InetAddress SubnetSite::getPivot()
{
    // Not refined yet or undefined: aborts
    if(this->refinementStatus == SubnetSite::NOT_PREPARED_YET ||
       this->refinementStatus == SubnetSite::UNDEFINED_SUBNET ||
       this->refinementStatus == SubnetSite::INCOMPLETE_SUBNET)
    {
        return InetAddress(0);
    }
    
    // Shadow subnet: gets the IP of the first "visible" address
    if(this->refinementStatus == SubnetSite::SHADOW_SUBNET)
    {
        return IPlist.front()->ip;
    }
    
    unsigned short shortestTTL = this->refinementTTL1;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        if((*i) != NULL && (*i)->TTL == shortestTTL + 1 && !(*i)->addedAtFilling)
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
    unsigned char prefixLength1 = ss1->getInferredSubnetPrefixLength();
    unsigned long int prefix1 = (ss1->getPivotAddress()).getULongAddress();
    prefix1 = prefix1 >> (32 - prefixLength1);
    prefix1 = prefix1 << (32 - prefixLength1);
    
    unsigned char prefixLength2 = ss2->getInferredSubnetPrefixLength();
    unsigned long int prefix2 = (ss2->getPivotAddress()).getULongAddress();
    prefix2 = prefix2 >> (32 - prefixLength2);
    prefix2 = prefix2 << (32 - prefixLength2);
    
    if (prefix1 < prefix2)
        return true;
    return false;
}

bool SubnetSite::compareAlt(SubnetSite *ss1, SubnetSite *ss2)
{
    unsigned long int initPrefix1 = (ss1->getPivotAddress()).getULongAddress();
    unsigned long int prefix1 = initPrefix1 >> 12;
    prefix1 = prefix1 << 12;
    
    unsigned long int initPrefix2 = (ss2->getPivotAddress()).getULongAddress();
    unsigned long int prefix2 = initPrefix2 >> 12;
    prefix2 = prefix2 << 12;
    
    if(prefix1 < prefix2)
    {
        return true;
    }
    else if(prefix1 == prefix2)
    {
        unsigned char prefixLength1 = ss1->getInferredSubnetPrefixLength();
        unsigned long int prefix1Bis = initPrefix1 >> (32 - prefixLength1);
        prefix1Bis = prefix1Bis << (32 - prefixLength1);
        
        unsigned char prefixLength2 = ss2->getInferredSubnetPrefixLength();
        unsigned long int prefix2Bis = initPrefix2 >> (32 - prefixLength2);
        prefix2Bis = prefix2Bis << (32 - prefixLength2);
        
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
        InetAddress pivotIP = ss2->getPivotAddress();
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
        if(this->refinementTTL1 == this->refinementTTL2)
            return true;
    
        // Checking contra-pivot IPs
        unsigned char contrapivotTTL = this->refinementTTL1;
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

bool SubnetSite::isAnArtifact()
{
    if(this->inferredSubnetPrefix >= (unsigned char) 32)
        return true;
    return false;
}

string SubnetSite::toString()
{
    stringstream ss;
    unsigned short status = refinementStatus;
    
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
        
            /*
             * Ignores nodes for which the registered prefix is smaller than the prefix of this site.
             * This condition is present in the original ExploreNET.
             */
            
            if((*i)->prefix >= inferredSubnetPrefix)
            {
                if(guardian)
                    ss << ", ";
                else
                    guardian = true;
            
                ss << (*i)->ip << " - " << (unsigned short) (*i)->TTL;
            }
            
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
    
    unsigned short baseTTL = (unsigned short) this->refinementTTL1;
    unsigned short diffTTL = (unsigned short) this->refinementTTL2 - baseTTL;
    
    if(diffTTL == 0)
    {
        return false;
    }
    
    unsigned int *interfacesByTTL = new unsigned int[diffTTL + 1];
    for (unsigned short i = 0; i < (diffTTL + 1); i++)
        interfacesByTTL[i] = 0;
    
    unsigned int totalInterfaces = 0;
    for(list<SubnetSiteNode*>::iterator i = IPlist.begin(); i != IPlist.end(); ++i)
    {
        /*
         * Ignores nodes for which the registered prefix is smaller than the prefix of this site.
         * This condition is present in the original ExploreNET.
         *
         * (*i) != NULL is there just in case, to avoid potential seg fault (rare, but seems to
         * occur on some PlanetLab nodes for some reason).
         */
        
        if((*i) != NULL && (*i)->prefix >= this->inferredSubnetPrefix)
        {
            unsigned short offset = (unsigned short) (*i)->TTL - baseTTL;
            if(offset <= diffTTL)
            {
                interfacesByTTL[offset]++;
                totalInterfaces++;
            }
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
    else if(diffTTL > 1)
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