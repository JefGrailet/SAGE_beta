/*
 * SubnetRefiner.h
 *
 *  Created on: Oct 23, 2014
 *      Author: jefgrailet
 *
 * This class gathers refinement methods to call on subnets after the scanning (in other words, it 
 * is a form of post-processing). The refinement methods are described just below.
 *
 * 1) Subnet filling
 *
 * During subnet inference, it is not rare to see that the IP list of some subnet misses some 
 * IPs which were previously considered as responsive. The goal of filling is to double check the 
 * list such that responsive interfaces encompassed by inferred subnets are listed in them as 
 * well at the end of the subnet refinement.
 *
 * 2) Subnet expansion
 *
 * This method consists in expanding a subnet which does not list a contra-pivot (i.e. all 
 * listed IPs are at the same hop count) and probing all non-listed IPs with a TTL smaller than 
 * one hop than the smallest TTL to get an echo reply from a contra-pivot. Indeed, by the 
 * definition used by ExploreNET, any sound subnet should have at least one contra-pivot 
 * interface. Once this interface has been found, the expansion stops and the expanded subnet is 
 * edited so that its prefix is the prefix for which the contra-pivot was found (contra-pivot is 
 * added in the listed IPs as well). When the expansion overgrowths the subnet (larger than /20), 
 * it stops as well to prevent ill cases.
 */

#ifndef SUBNETREFINER_H_
#define SUBNETREFINER_H_

#include "../../Environment.h"
#include "../../structure/SubnetSite.h"
#include "../../structure/SubnetSiteSet.h"
#include "ProbesDispatcher.h"

class SubnetRefiner
{
public:

    static const unsigned short LOWEST_PREFIX_ALLOWED = 20;
    static const unsigned short MAX_CONTRAPIVOT_CANDIDATES = 5;

    // Constructor, destructor
    SubnetRefiner(Environment *env);
    ~SubnetRefiner();
    
    // Refinement methods
    void expand(SubnetSite *ss);
    void fill(SubnetSite *ss);
    
    /*
     * Expansion method to use for shadow subnets at the end of the scanning ONLY; give them 
     * the greatest possible size without colliding with other (accurate/odd) subnets.
     */
    
    void shadowExpand(SubnetSite *ss);
    
private:

    // Pointer to the environment singleton (gives access to the subnet set and prober parameters)
    Environment *env;
    
    // Debug stuff (N.B.: no log because there is no concurrent execution of expand())
    bool printSteps, debug;
    
    /*
     * November 2017: probes all IPs (responsive at pre-scanning) not in the subnet which are 
     * encompassed in its boundaries with the pivot TTL (due to expansion, there can be no other 
     * IP in the subnet with contra-pivot TTL or lower). This only occurs after a successful 
     * expansion into an accurate/odd subnet.
     */
    
    void finalize(SubnetSite *ss);
}; 

#endif /* SUBNETREFINER_H_ */
