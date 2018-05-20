/*
 * Node.h
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * This extension of Neighborhood models a single neighborhood where all subnets share the same 
 * penultimate hop (which act as a label, much like in a NetworkTreeNode in TreeNET), contrary to 
 * the Cluster class which models subnets which have penultimate hops that could be aliased 
 * together. It is a concrete class.
 */

#ifndef NODE_H_
#define NODE_H_

#include "Neighborhood.h"
#include "../Aggregate.h"

class Node : public Neighborhood
{
public:

    // Constructor/destructor
    Node(Aggregate *a);
    ~Node();
    
    inline InetAddress getLabel() { return label; }
    
    list<InetAddress> listAllInterfaces();
    string getFullLabel();
    string toString();

protected:

    InetAddress label;
    unsigned short labelAnomalies;
    
    /*
     * The padding field is there just to avoid potential confusion between a Node with label A 
     * and another Node with label A padded with two missing hops.
     */

};

#endif /* NODE_H_ */
