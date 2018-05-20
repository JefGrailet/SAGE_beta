/*
 * SubnetTracer.h
 *
 *  Created on: Sept 22, 2017
 *      Author: jefgrailet
 *
 * This class compiles together all operations related traceroute phase, which includes the 
 * measurement itself and attempts at fixing the missing hops by completing them with additionnal 
 * probes or comparison of routes. It is named "SubnetTracer" because if traces the route to each 
 * subnet observed during network scanning.
 */

#ifndef SUBNETTRACER_H_
#define SUBNETTRACER_H_

#include "../Environment.h"

class SubnetTracer
{
public:

    // Constructor, destructor
    SubnetTracer(Environment *env);
    ~SubnetTracer();
    
    void measure(); // Schedules ParisTracerouteTask threads
    void repair(); // Looks for incomplete routes and attempts to correct them

private:
    
    // Pointer to the environment singleton
    Environment *env;
    
    /**** Private methods for route repairment and analysis ****/
    
    // Method to count the amount of incomplete routes seen in the set of subnets.
    unsigned int countIncompleteRoutes();
    
    // Repairment of a route (see prepare()); returns the amount of replaced missing hops.
    unsigned short repairRouteOffline(SubnetSite *ss);
    
    // At the end of repairment, changes placeholder IPs (0.0.0.0/24) back to 0.0.0.0 (missing hop)
    void postProcessRoutes();
    
};

#endif /* SUBNETTRACER_H_ */
