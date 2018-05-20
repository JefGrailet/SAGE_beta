/*
 * Edge.h
 *
 *  Created on: Oct 3, 2017
 *      Author: jefgrailet
 *
 * Models a (directed) edge in a graph. It's the superclass of several types of edges.
 */

#ifndef EDGE_H_
#define EDGE_H_

// Inclusion with forward declaration
#include "Neighborhood.h"
class Neighborhood;

class Edge
{
public:

    static const unsigned short DIRECT_LINK = 0;
    static const unsigned short INDIRECT_LINK = 1;
    static const unsigned short REMOTE_LINK = 2;

    Edge(unsigned short type, Neighborhood *tail, Neighborhood *head);
    virtual ~Edge();
    
    inline unsigned short getType() { return type; }
    inline Neighborhood* getTail() { return tail; }
    inline Neighborhood* getHead() { return head; }
    
    // To be implemented by child class
    virtual string toString() = 0;
    virtual string toStringVerbose() = 0;

protected:
    
    // Type field; used to see what kind it is (compensates the absence of an instanceof operator)
    unsigned short type;
    
    // In graph theory: given an edge u -> v, u is the tail, while v is the head
    Neighborhood *tail, *head;
    
    /*
     * N.B.: head is never displayed in toString() methods. The main motivation is to be able to 
     * travel backwards in the graph, and nothing else.
     */
    
};

#endif /* EDGE_H_ */
