/*
 * Graph.h
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * As the name suggests, this class models a neighborhood graph.
 */

#ifndef GRAPH_H_
#define GRAPH_H_

#include <list>
using std::list;
#include <map>
using std::map;

#include "./components/Neighborhood.h"
#include "./components/MiscHop.h"

// Inclusion with forward declaration
#include "SubnetMapEntry.h"
class SubnetMapEntry;

class Graph
{
public:

    /*
     * Size of the subnetMap array (array of lists), which is used for fast look-up of a subnet 
     * listed in a Neighborhood class on the basis of an interface it should contain. Its size is 
     * based on the fact that no subnet of a prefix length shorter than /20 was ever found in the 
     * measurements. Therefore, the 20 first bits of any interface in a subnet are used to access 
     * a list of subnets sharing the same prefix in O(1), the list containing at most 2048 subnets 
     * (2048 subnets of prefix length /31). This dramatically speeds up the look-up for a subnet 
     * present in a graph (in comparison with a more trivial method where one visits the graph), 
     * at the cost of using more memory (~8Mo).
     */

    const static unsigned int SIZE_SUBNET_MAP = 1048576;

    // Constructor, destructor
    Graph();
    ~Graph();
    
    // Methods to handle gates and amount of neighborhoods
    inline list<Neighborhood*> *getGates() { return &gates; }
    inline unsigned int getNbNeighborhoods() { return nbNeighborhoods; }
    inline void addGate(Neighborhood *gate) { gates.push_back(gate); }
    inline void setNbNeighborhoods(unsigned int nb) { nbNeighborhoods = nb; }
    
    // Gets a miscellaenous hop (IP or 0.0.0.0 with TTL), or creates it if does not exist
    MiscHop *getMiscHop(InetAddress IP);
    MiscHop *getMiscHop(unsigned char TTL);
    
    // Gets all MiscHop objects
    list<MiscHop*> getMiscHops();
    
    // Inserts the subnets of a neighborhood as subnet map entries and sorts all lists of the map
    void mapSubnetsFrom(Neighborhood *n);
    void sortMapEntries();
    
    // Gets a subnet found in the graph which contains the given input address (NULL if not found).
    SubnetMapEntry *getSubnetContaining(InetAddress needle);
    
private:

    /*
     * Private fields:
     * -gates: Neighborhood objects that first appear in the topology with respect to the 
     *  traceroute measurements.
     * -miscHops: a map of nodes which correspond to IPs which have no associated neighborhood 
     *  but appeared in traceroute nevertheless. They are used to build a second graph which is 
     *  meant to connect neighborhoods which are remote peers of each other.
     * -missingHops: same purpose as miscHops, but for 0.0.0.0's, indexed by TTL.
     * -subnet map: array of lists for fast subnet look-up (cf. large comment in public section).
     */
    
    list<Neighborhood*> gates;
    map<InetAddress, MiscHop*> miscHops;
    map<unsigned char, MiscHop*> missingHops;
    list<SubnetMapEntry*> *subnetMap;
    
    // Total amount of neighborhood nodes; set after a visit by Pioneer
    unsigned int nbNeighborhoods;

};

#endif /* GRAPH_H_ */
