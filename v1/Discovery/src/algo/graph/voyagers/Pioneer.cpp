/*
 * Pioneer.cpp
 *
 *  Created on: Nov 16, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Pioneer.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "Pioneer.h"

Pioneer::Pioneer(Environment *env) : Voyager(env)
{
    counter = 1;
}

Pioneer::~Pioneer()
{
}

void Pioneer::visit(Graph *g)
{
    list<Neighborhood*> *gates = g->getGates();
    for(list<Neighborhood*>::iterator i = gates->begin(); i != gates->end(); ++i)
        this->visitRecursive((*i));
    
    g->setNbNeighborhoods(counter - 1);
}

void Pioneer::visitRecursive(Neighborhood *n)
{
    if(n->getID() != 0)
        return;

    n->setID(counter);
    counter++;
    
    list<Edge*> *next = n->getOutEdges();
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
        this->visitRecursive((*i)->getHead());
}
