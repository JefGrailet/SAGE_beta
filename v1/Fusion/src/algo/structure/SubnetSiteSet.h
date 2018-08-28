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
 *
 * N.B.: the class slightly differs from its equivalent in "Discovery". Indeed, due to the absence 
 * of subnet inference in "Fusion", some methods were irrelevant (and could not be kept due to 
 * SubnetSite also being different).
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
    
    // Method to add a new subnet with neither merging, nor sorting.
    void addSiteNoMerging(SubnetSite *ss);
    
    // Method to sort the set (to use in complement with addSiteNoMerging()).
    void sortSet();
    
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
    
private:

    // Sites are stored with a list
    list<SubnetSite*> siteList;
};

#endif /* SUBNETSITESET_H_ */
