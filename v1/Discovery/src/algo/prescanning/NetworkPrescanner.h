/*
 * NetworkPrescanner.h
 *
 *  Created on: Oct 8, 2015
 *      Author: jefgrailet
 *
 * NetworkPrescanner is a class designed to send a probe to a given list of IP addresses and save 
 * the responsive ones in the IP dictionnary (IPLookUpTable). Unresponsive IPs are listed by the 
 * same class as well, in order to perform a second/third/... opinion probing, most probably with 
 * different parameters (e.g.: timeout period). These parameters are saved for each responsive IP 
 * in the look-up table to ensure getting a response from them in the subsequent probing steps.
 */

#ifndef NETWORKPRESCANNER_H_
#define NETWORKPRESCANNER_H_

#include <list>
using std::list;

#include "../Environment.h"

class NetworkPrescanner
{
public:

    const static unsigned short MINIMUM_TARGETS_PER_THREAD = 2;

    // Constructor, destructor
    NetworkPrescanner(Environment *env);
    ~NetworkPrescanner();
    
    // Methods to configure the prescanner
    inline void setTimeoutPeriod(TimeVal timeout) { this->timeout = timeout; }
    bool hasUnresponsiveTargets();
    void reloadUnresponsiveTargets();
    
    // Accesser to timeout field (for children threads)
    inline TimeVal getTimeoutPeriod() { return this->timeout; }
    
    // Callback method (for children threads)
    void callback(InetAddress target, bool responsive);
    
    // Launches a pre-scanning (i.e. probing once all IPs from targets)
    void probe();
    
    // Performs the whole pre-scanning phase. This slightly simplifies Main.cpp.
    void run(list<InetAddress> targets);
    
private:

    // Pointer to the environment singleton (provides access to the subnet set and prober parameters)
    Environment *env;
    
    // Timeout period for the current scan
    TimeVal timeout;
    
    // List of targets
    list<InetAddress> targets;
    list<InetAddress> unresponsiveTargets; // From a previous probing, thus empty at first
    
    /*
     * N.B.: responsive targets are not stored in a third list, and directly saved in the IP 
     * look-up table (which is accessible through env).
     */
}; 

#endif /* NETWORKPRESCANNER_H_ */
