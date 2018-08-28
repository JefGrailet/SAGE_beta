/*
 * SubnetSiteSet.h
 *
 *  Created on: Oct 9, 2014
 *      Author: jefgrailet
 *
 * A simple class to gather subnets and organize them (by merging overlapping subnets, for 
 * instance) before further discovery steps. Sites registered in the set are sorted according to 
 * their CIDR notation (small IPs first). When a new subnet happens to include a previously 
 * registered subnet, the "old" subnet is removed and the lists of IPs of both are merged 
 * together. Reciprocally, when a subnet to add is encompassed by an already registered subnet, 
 * the new subnet is not inserted but its IPs missing from the already registered subnet are 
 * stored within the object of the latter.
 */

#ifndef SUBNETSITESET_H_
#define SUBNETSITESET_H_

#include <list>
using std::list;
#include <string>
using std::string;

#include "../../common/inet/InetAddress.h"
#include "SubnetSite.h"

class SubnetSiteSet
{
public:

    // Possible results when adding a site
    enum UpdateResult
    {
        KNOWN_SUBNET, // Site already in the set (in practice, for /32 subnets)
        SMALLER_SUBNET, // Site already in the set, but with bigger/equivalent prefix
        BIGGER_SUBNET, // Site already in the set, but with smaller prefix
        NEW_SUBNET // Site not in the set
    };

    SubnetSiteSet();
    ~SubnetSiteSet();
    
    inline list<SubnetSite*> *getSubnetSiteList() { return &siteList; }
    inline unsigned int getNbSubnets() { return siteList.size(); }
    
    // Adds a new subnet or merges it with an already listed subnet.
    unsigned short addSite(SubnetSite *ss);
    
    /*
     * Method to test if an hypothetical subnet (represented by its borders and pivot TTL) is 
     * compatible with this set. A subnet being compatible with the set means that it either does 
     * not overlap anything, either only overlaps subnets which the pivot TTL is the same.
     *
     * A boolean is required to tell if we should check both TTL-1 and TTL+1 to ensure the TTL is 
     * similar, for when the subnet consists of only one interface. The boolean shadowExpansion 
     * should be set to true to prevent an expanding SHADOW subnet from overlapping ACCURATE/ODD 
     * subnets (see finalize() in the class SubnetRefiner).
     */
     
    bool isCompatible(InetAddress lowerBorder, 
                      InetAddress upperBorder, 
                      unsigned char TTL, 
                      bool beforeAndAfter, 
                      bool shadowExpansion);
    
    // Gets a shadow subnet and removes it from the list (NULL if none)
    SubnetSite *getShadowSubnet();
    
    // Gets a subnet with a valid route (complete = no missing hop) and removes it (NULL if none)
    SubnetSite *getValidSubnet(bool completeRoute = true);
    
    // Removes subnets which the prefix is still 32 after refinement
    void removeArtifacts();
    
    // Gets the maximum TTL value to a pivot IP in the set
    unsigned short getMaximumDistance();
    
    // When routes are available, sorts subnets by (increasing) route size
    void sortByRoute();
    
    // Writes the complete set in an output file of a given name.
    void outputAsFile(string filename);
    
    /*
     * January 2016: methods to test whether a subnet or IP is encompassed by a subnet already in 
     * the set with the same pivot TTL. The goal of is to test if such an IP or subnet (to refine 
     * with expansion) is encompassed by some undefined subnet (listed in "IPBlocksToAvoid" field 
     * in Environment singleton) which the pivot TTL is the same, in order to avoid unnecessary 
     * and redundant probing. Both methods returns NULL if the IP/subnet is not encompassed, 
     * otherwise the encompassing subnet is returned.
     */
    
    SubnetSite *getEncompassingSubnet(InetAddress ip, unsigned char TTL);
    SubnetSite *getEncompassingSubnet(SubnetSite *ss);
    
    /*
     * November 2017: methods to prepare and finalize the set before and after a refinement round 
     * during network scanning (i.e., "bypass"). The first method travels the set to find all 
     * unrefined subnets added during previous probing round, determine their classification 
     * (i.e., accurate, odd, incomplete) and mark them as new. It returns a list of the incomplete 
     * subnets along a list of all newly discovered subnets (via pointer) for display purpose.
     * 
     * The second method visits the set again to get all remaining "new" subnets (as refinement 
     * and merging should have reduced the amount of subnets) and unmark them. This list is a 
     * pre-requisite for the new target filtering heuristic (cf. NetworkScanner.cpp).
     *
     * It should be noted that while the first method removes the incomplete subnets from the set 
     * before returning them, the second leaves untouched in the set.
     */
    
    list<SubnetSite*> postProcessNewSubnets(list<SubnetSite*> *discovered);
    list<SubnetSite*> listNewAndRefinedSubnets();
    
private:

    // Sites are stored with a list
    list<SubnetSite*> siteList;
};

#endif /* SUBNETSITESET_H_ */
