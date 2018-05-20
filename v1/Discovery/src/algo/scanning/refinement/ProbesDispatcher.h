/*
 * ProbesDispatcher.h
 *
 *  Created on: Oct 30, 2014
 *      Author: jefgrailet
 *
 * Provided a list of IPs to probe (ping-like), this class is used to dispatch blocks of IPs (of a 
 * fixed size) between several threads with their own prober instance in order to speed up the
 * probing of a large amount of IPs (such as a range).
 */

#ifndef PROBESDISPATCHER_H_
#define PROBESDISPATCHER_H_

#include <list>
using std::list;

#include "../../Environment.h"

class ProbesDispatcher
{
public:

    static const unsigned short MINIMUM_IPS_PER_THREAD = 2;
    
    // Possible results upon completing the probing work
    enum DispatchResult
    {
        FOUND_ALTERNATIVE,
        FOUND_RESPONSIVE_IPS,
        FOUND_PROOF_TO_DISCARD_ALTERNATIVE,
        FOUND_NOTHING
    };

    // Constructor (args are for the ICMP probers), destructor
    ProbesDispatcher(Environment *env, 
                     std::list<InetAddress> IPsToProbe,
                     unsigned char requiredTTL,
                     unsigned char alternativeTTL = 0);
    ~ProbesDispatcher();
    
    // Dispatch method
    unsigned short dispatch();
    
    // Accesser to responsive IPs list and "foundAlternative" flag (for probe units)
    inline std::list<InetAddress> *getResponsiveIPs() { return &responsiveIPs; }
    inline bool hasFoundAlternative() { return foundAlternative; }
    inline bool ignoringAlternative() { return ignoreAlternative; }
    
    // Method to set "foundAlternative" and "ignoreAlternative" to true (used by probe units)
    inline void raiseFoundAlternativeFlag() { foundAlternative = true; }
    inline void raiseIgnoreAlternativeFlag() { ignoreAlternative = true; }
    
private:

    // Pointer to the environment singleton
    Environment *env;

    // Very own private fields
    std::list<InetAddress> IPsToProbe, responsiveIPs;
    unsigned char requiredTTL, alternativeTTL;
    
    /*
     * Note on alternativeTTL: this is a TTL value which should also be probed if requiredTTL does 
     * not work. In practice, it's set at subnet expansion to pivot TTL + 1 if the initial subnet 
     * contains only one IP (therefore, we have to ensure it's not a contra-pivot itself). If set 
     * to zero, the alternative probe will not be sent. Flags (see below) are used to know if a 
     * ProbeUnit got a successful probe at the alternative TTL or at regular TTL. These flags are 
     * checked by each ProbeUnit before sending a new probe with the alternative TTL to avoid 
     * unnecessary probing work.
     */
    
    bool foundAlternative; // Flag telling alternative has been found
    bool ignoreAlternative; // Flag telling to not look for alternative anymore
};

#endif /* PROBESDISPATCHER_H_ */
