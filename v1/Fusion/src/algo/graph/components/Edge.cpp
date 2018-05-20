/*
 * Edge.cpp
 *
 *  Created on: Oct 3, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Edge.h (see this file to learn further about the goals of such 
 * a class).
 */

#include "Edge.h"

Edge::Edge(unsigned short type, Neighborhood *tail, Neighborhood *head)
{
    this->type = type;
    this->tail = tail;
    this->head = head;
}

Edge::~Edge()
{
    // Neighborhoods should be deleted via another method
}
