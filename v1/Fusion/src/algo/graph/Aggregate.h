/*
 * Aggregate.h
 *
 *  Created on: Sept 28, 2017
 *      Author: jefgrailet
 *
 * This class models an aggregate of subnets, created before building the graph. The idea is to 
 * have a first set of "proto-neighborhoods" where all gathered strictly have the same 
 * neighborhood label. Indeed, the graph structure allows "clusters", neighborhoods where the 
 * IPs of the neighborhood labels are aliases of each other.
 */

#ifndef AGGREGATE_H_
#define AGGREGATE_H_

#include <list>
using std::list;

#include "./Peer.h"
#include "../structure/SubnetSite.h"

class Aggregate
{
public:

    // Constructor/destructor
    Aggregate(list<SubnetSite*> subnets);
    ~Aggregate();
    
    // Accessers
    inline list<SubnetSite*> *getSubnets() { return &subnets; }
    inline list<unsigned char> *getTTLs() { return &TTLs; }
    inline list<Peer*> *getPeers() { return &peers; }
    inline unsigned short getNbPeers() { return (unsigned short) peers.size(); }
    
    inline InetAddress getLabelIP() { return subnets.front()->getNeighborhoodLabelIP(); }
    inline unsigned short getLabelAnomalies() { return subnets.front()->getNeighborhoodLabelAnomalies(); }
    inline string getLabelString() { return subnets.front()->getNeighborhoodLabelString(); }
    
    // Methods to handle the "junction" flag
    inline bool isAJunction() { return junction; }
    inline void markAsJunction() { junction = true; }
    
    // toString() method, essentially for debug
    string toString();

private:

    list<SubnetSite*> subnets;
    list<unsigned char> TTLs;
    list<Peer*> peers; // Potential peers for the neighborhood equivalent to this aggregate
    
    /*
     * Boolean flag set to true if the aggregate is used to create a "concrete peer" during step 3 
     * of graph building. Having a "junction" flag set to false means that the aggregate is a 
     * "terminus" neighborhood of the graph (i.e., it appears at its extremities and does not act 
     * as a junction towards farther neighborhoods).
     */
    
    bool junction;
    
    // Simple private method to test if some TTL is already listed
    bool isTTLListed(unsigned char TTL);

};

#endif /* AGGREGATE_H_ */
