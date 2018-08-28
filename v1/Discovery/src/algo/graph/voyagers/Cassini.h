/*
 * Cassini.h
 *
 *  Created on: Nov 29, 2017
 *      Author: jefgrailet
 *
 * This voyager visits a graph to compute a variety of early metrics, including various notions of 
 * degree and the amount of nodes that can be reached for each gate. A boolean array is used to 
 * avoid visiting a same node several times.
 */

#ifndef CASSINI_H_
#define CASSINI_H_

#include "Voyager.h"

class Cassini : public Voyager
{
public:

    // Constructor, destructor
    Cassini(Environment *env);
    ~Cassini(); // Implicitely virtual
    
    void visit(Graph *g); // Implicitely virtual
    
    // Methods to get metrics in string format, or as an output file
    string getMetrics();
    void outputMetrics(string filename);

protected:

    // Array for avoiding visiting a node more than once
    bool *visited;
    unsigned int nbToVisit;
    
    list<unsigned int> reachableNodes;
    list<list<unsigned int> > visitedGates;

    // Fields to handle the metrics
    float degreeAcc, totalNodes; // Acc for "accumulator"
    
    unsigned short maxInDegree;
    list<unsigned int> maxInIDs;
    
    unsigned short maxOutDegree;
    list<unsigned int> maxOutIDs;
    
    unsigned short minTotDegree, maxTotDegree;
    list<unsigned int> minTotIDs, maxTotIDs;

    unsigned short minNbSubnets, maxNbSubnets;
    list<unsigned int> minNbSubIDs, maxNbSubIDs;
    
    // Metrics about subnets
    unsigned int accSubnets, oddSubnets, shadSubnets, credSubnets;
    unsigned int coveredIPs, coveredIPsCred;
    
    // Metrics about "super-"neighborhoods (i.e., > 200 subnets)
    list<unsigned int> superClusters, superNodes;
    
    // Metrics about aliases
    unsigned int maxAliasAmount, nbSingleAlias;
    unsigned short maxAliasSize;
    list<unsigned int> maxAmountIDs;
    float totalAliases;
    
    // Metrics about edges
    unsigned int nbDirectLinks, nbIndirectLinks, nbRemoteLinks, nbLinksWithMedium;
    
    /*
     * N.B.: only the "in" degree is summed to degreeAcc during the visit. Indeed, in/out degrees 
     * are equal, since the total of edges will be the same in both cases. Moreover, the average 
     * total degree will consist in twice the total amount of edges divided by the amount of 
     * nodes, therefore average in (or out) degree times 2.
     */
     
    void reset(); // Resets all numeric fields to 0 and arrays to NULL pointers
    
    /*
     * Methods to recursively visit the graph, node by node.
     * -visitRecursive1() is meant to compute the minimum/maximum in/out degree along the 
     *  minimum/maximum amount of subnets per neighborhood. In other words, node metrics.
     * -visitRecursive2() is meant to go through a gate, then mark as visited any node connected 
     *  to the current component; i.e., it will also look at the in edges for each node. When the 
     *  method reaches a gate (i.e., no "in" edge), it appends it to the list "componentGates" 
     *  passed by address to the method.
     */
    
    void visitRecursive1(Neighborhood *n);
    void visitRecursive2(Neighborhood *n, list<unsigned int> *componentGates);

};

#endif /* CASSINI_H_ */
