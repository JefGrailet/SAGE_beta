/*
 * GraphBuilder.h
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * As the name suggests, the purpose of this class is to build a neighborhood graph. To this end, 
 * it solely relies to the environment singleton (to get the set of subnets) and progressively 
 * builds the neighborhoods and connect them. It uses a variety of private fields and methods to 
 * achieve this goal.
 */

#ifndef GRAPHBUILDER_H_
#define GRAPHBUILDER_H_

#include <string>
using std::string;

#include "Graph.h"
#include "components/Cluster.h"
#include "../aliasresolution/AliasHintCollector.h"
#include "../aliasresolution/AliasResolver.h"
#include "../Environment.h"

class GraphBuilder
{
public:

    // Constructor, destructor
    GraphBuilder(Environment *env);
    ~GraphBuilder();
    
    // Accessers to the result
    inline bool hasResult() { return this->result != NULL; }
    Graph *getResult();
    
    // Starts the building
    void build();
    
    // Checks if anomalies occurred, and prints them
    bool gotAnomalies();
    void outputAnomalies(string filename);
    
private:

    // Pointer to the environment builder
    Environment *env;
    
    // Pointers to AliasHintCollector & AliasResolver objects, used during building
    AliasHintCollector *ahc;
    AliasResolver *ar;
    
    // Resulting graph
    Graph *result;
    
    // stringstream to log problems; E.g., it's used if a peer has no concrete equivalent at step 4
    stringstream anomalies;
    
    /*
     * ---------------
     * Private methods
     * ---------------
     */
    
    /*
     * Checks if a given Aggregate object should eventually be connected, as a Neighborhood, to 
     * other neighborhoods appearing sooner or if it should be a "gate" of the graph (see comment 
     * above termini and junctions fields).
     *
     * @param Aggregate* a  The Aggregate object to check
     * @return bool         True if it should be eventually connected
     */
    
    bool isConnectionNeeded(Aggregate *a);
    
    /*
     * (Step 2) Estimates the peers for an aggregate as a whole. I.e., if there are several route 
     * lengths for the aggregated subnets, the method lists subnets per route length then computes 
     * peers for each sub-group and makes the synthesis of all possible peers (that is, if Peer 
     * objects with common IPs appear for each sub-group, they will be merged). This method relies 
     * on subsequent private methods identifyPeers() and postProcessPeers().
     *
     * @param Aggregate* a  The Aggregate object for which peers should be estimated
     */
    
    void estimatePeers(Aggregate *a);
    
    /*
     * (Step 2) Identify peers for a subset of subnets found in a same neighborhood. This private 
     * method involves alias resolution when there are multiple possible interfaces, in case these 
     * interfaces are actually from a same device.
     *
     * @param list<SubnetSite*> subnets  The subset of subnets to analyze
     * @return list<Peer*>               The identified peer(s)
     */
    
    list<Peer*> identifyPeers(list<SubnetSite*> subnets);
    
    /*
     * (Step 2 & 3) Post-processes the peers identified for an aggregate. Indeed, it is possible, 
     * in case of multiple route lengths or after peer aggregation, that there are redundant 
     * peers. This method takes a look at the list of peers and check if any is similar to others, 
     * and if yes, it proceeds to merge the lists of IPs (if not equal) and deletes the redudant 
     * Peer objects.
     *
     * @param Aggregate* a  The Aggregate object for which peers should be post-processed
     */
    
    void postProcessPeers(Aggregate *a);
    
    /*
     * (step 3) Post-processes the peers of a Cluster object. It exclusively looks for duplicate 
     * peers and remove them from the list. Doing this post-processing at the instantiation of a 
     * Cluster can invalidate pointers in the final map of peers during the creation of concrete 
     * peers.
     *
     * @param Cluster* c  The Cluster object for which peers should be post-processed
     */
    
    void postProcessPeers(Cluster *c);
    
    /*
     * (Step 3) Records aliases discovered during step 2 & 3 (via alias resolution + peer 
     * aggregation) as "pre-aliases" in the IP dictionnary.
     *
     * @param Peer* peer  The alias (as a multi-IPs peer) to record
     */
    
    void registerPreAlias(Peer *peer);
    
    /*
     * (Step 4) Connects a neighborhood to its peers with new edges, which are either direct, 
     * indirect or remote depending on the situation. Once all edges with the peers are 
     * established, the method is called recursively on the concrete peers.
     *
     * @param Neighborhood* n  The neighborhood for which we want to connect with its peers
     */
    
    void connectToPeers(Neighborhood *n);
    
    /*
     * (Step 5) Numbers a neighborhood and its peers (reached via Edge objects; there should be no 
     * longer any Peer object at numbering).
     * 
     * @param Neighborhood* n  The neighborhood to number (peers recursively numbered too)
     */
    
    void number(Neighborhood *n);

};

#endif /* GRAPHBUILDER_H_ */
