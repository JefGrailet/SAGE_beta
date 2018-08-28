/*
 * Peer.h
 *
 *  Created on: Oct 12, 2017
 *      Author: jefgrailet
 *
 * This class models a "peer" of a neighborhood, i.e., the next IP interface/alias corresponding 
 * to another neighborhood that can be reached from a given neighborhood. A peer is derived by 
 * looking at a set of routes (of the same length) towards subnets of a neighborhood, finding the 
 * closest TTL to penultimate hop where one or several IPs (penultimate hops) of other aggregates 
 * appear, listing all possibilities at that TTL and then conducting alias resolution to ensure 
 * the different possible interfaces aren't from a same device (or several devices). In the end, 
 * a peer should always contain at least one penultimate hop from another aggregate.
 */

#ifndef PEER_H_
#define PEER_H_

#include "../structure/Router.h"

#include "./components/Neighborhood.h"
class Neighborhood;

class Peer
{
public:

    // Constructors and destructor
    Peer(unsigned short offset, InetAddress junction);
    Peer(unsigned short offset, Router *alias);
    ~Peer();
    
    // Accessers and one setter
    inline list<InetAddress> *getInterfaces() { return &interfaces; }
    inline unsigned short getOffset() { return offset; }
    inline Neighborhood *getConcretePeer() { return concretePeer; }
    inline void setConcretePeer(Neighborhood *concrete) { concretePeer = concrete; }
    
    // Simple method to check a Peer object lists a given interface
    bool contains(InetAddress interface);
    
    // Comparison, merging
    bool isSimilarTo(Peer *p);
    bool equals(Peer *p);
    void mergeWith(Peer *mergee); // Mergee is not deleted by this method
    static bool compare(Peer *p1, Peer *p2);
    
    string toString(bool showOffset = true);
    
private:
    
    list<InetAddress> interfaces;
    unsigned short offset; // Amount of hops between this peer and the penultimate hop of parent
    Neighborhood *concretePeer; // Initially NULL, it is set during peer aggregation
    
};

#endif /* PEER_H_ */
