/*
 * Pioneer.h
 *
 *  Created on: Nov 16, 2017
 *      Author: jefgrailet
 *
 * This simple voyager visits a graph to list its neighborhoods. A boolean array is used to avoid 
 * visiting a same node several times.
 */

#ifndef MARINER_H_
#define MARINER_H_

#include "Voyager.h"

class Mariner : public Voyager
{
public:

    // Constructor, destructor
    Mariner(Environment *env);
    ~Mariner(); // Implicitely virtual
    
    void visit(Graph *g); // Implicitely virtual
    
    // Methods to handle the lists
    inline list<Neighborhood*> *getNodesList() { return &nodes; }
    inline list<MiscHop*> *getMiscList() { return &misc; }
    inline void cleanNodesList() { nodes.clear(); }
    inline void cleanMiscList() { misc.clear(); }
    
    // Methods to output the neighborhoods and the graph
    void outputNeighborhoods(string filename);
    void outputGraph(string filename);
    
    // Mariner is also used to delete the listed neighborhoods
    void cleanNeighborhoods();

protected:

    // Listed nodes
    list<Neighborhood*> nodes;
    list<MiscHop*> misc;
    
    // Array for avoiding visiting a node more than once
    bool *visited;
    unsigned int nbToVisit;
    
    // Method to recursively visit the graph, node by node.
    void visitRecursive(Neighborhood *n);

};

#endif /* MARINER_H_ */
