/*
 * Neighborhood.h
 *
 *  Created on: Sept 26, 2017
 *      Author: jefgrailet
 *
 * This class models a neighborhood. Unlike the NetworkTreeNode class in TreeNET, the Neighborhood 
 * class here truly models a single neighborhood, consisting of a list of SubnetSite objects which 
 * share the same neighborhood label(s). It is extended into the Node and Cluster classes 
 * (respectively single-label and multi-label neighborhoods; the latter are only created when said 
 * labels are aliases of each other).
 */

#ifndef NEIGHBORHOOD_H_
#define NEIGHBORHOOD_H_

#include "../../structure/SubnetSite.h"
#include "../../aliasresolution/Fingerprint.h"

#include "../Peer.h" // Also indirectly gives access Router.h
class Peer;

#include "Edge.h"
class Edge;

class Neighborhood
{
public:

    // Constructor/destructor
    Neighborhood();
    virtual ~Neighborhood();
    
    // Accessers
    inline unsigned int getID() { return ID; }
    inline list<Edge*> *getInEdges() { return &inEdges; }
    inline list<Edge*> *getOutEdges() { return &outEdges; }
    inline list<SubnetSite*> *getSubnets() { return &subnets; }
    inline list<Fingerprint> *getFingerprints() { return &fingerprints; }
    inline list<Router*> *getAliases() { return &aliases; }
    inline list<Peer*> *getPeers() { return &peers; }
    inline unsigned short getNbPeers() { return (unsigned short) peers.size(); }
    
    // Setter
    inline void setID(unsigned int ID) { this->ID = ID; }
    
    // Methods to handle connections to this vertice
    inline void addInEdge(Edge *e) { inEdges.push_back(e); }
    inline void addOutEdge(Edge *e) { outEdges.push_back(e); }
    bool isConnectedTo(Neighborhood *n);
    
    // Methods to deal with various metrics
    inline unsigned short getInDegree() { return (unsigned short) inEdges.size(); }
    inline unsigned short getOutDegree() { return (unsigned short) outEdges.size(); }
    inline unsigned short getDegree() { return (unsigned short) (inEdges.size() + outEdges.size()); }
    inline unsigned short getNbSubnets() { return (unsigned short) subnets.size(); }
    inline bool canHaveAliases() { return fingerprints.size() > 0; }
    inline unsigned int getNbAliases() { return aliases.size(); }
    inline bool isOneAlias() { return (fingerprints.size() > 1 && aliases.size() == 1); }
    
    void getSubnetMetrics(unsigned int *metrics); // 0=#accurate, 1=#odd, 2=#shadow, 3=#credible
    unsigned short getLargestAliasSize();
    
    // Returns the subnet containing nextHop, NULL otherwise
    SubnetSite *getConnectingSubnet(InetAddress nextHop);
    
    // Sorting method
    static bool smallerID(Neighborhood *n1, Neighborhood *n2);
    
    virtual list<InetAddress> listAllInterfaces() = 0;
    virtual string getFullLabel() = 0;
    virtual string toString() = 0;

protected:

    // Main fields
    unsigned int ID; // To be set after the graph has been fully generated, for output purpose only
    list<Edge*> inEdges, outEdges;

    // Data fields
    list<SubnetSite*> subnets;
    list<Fingerprint> fingerprints;
    list<Router*> aliases;
    
    // Fields used during graph building
    list<Peer*> peers;

};

#endif /* NEIGHBORHOOD_H_ */
