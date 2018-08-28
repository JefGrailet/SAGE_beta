/*
 * Cluster.h
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * This extension of Neighborhood models a neighborhood which subnets have different penultimate 
 * hops that could be aliased together. Clusters appear when there are several possibilities for 
 * the previous hops before a precise step in a route. In such a case, it is safer to conduct 
 * alias resolution to ensure the multiple possibilities are from distinct devices or from a same 
 * device. When aliases are discovered, the aggregates for each of the aliased IPs are merged into 
 * a single neighborhood, therefore identified as a "cluster".
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_

#include "Neighborhood.h"
#include "../Aggregate.h"

class Cluster : public Neighborhood
{
public:

    // Constructor/destructor
    Cluster(list<Aggregate*> aggregates);
    ~Cluster();
    
    inline list<InetAddress> *getLabels() { return &labels; }
    
    void addLabel(InetAddress alias); // Adds an IP to labels list
    
    list<InetAddress> listAllInterfaces();
    string getFullLabel();
    string toString();

protected:
    
    list<InetAddress> labels;
    list<InetAddress> junctionLabels; // List labels appearing as penultimate hops
    
};

#endif /* CLUSTER_H_ */
