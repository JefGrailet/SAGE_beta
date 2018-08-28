/*
 * Pioneer.h
 *
 *  Created on: Nov 16, 2017
 *      Author: jefgrailet
 *
 * This simple voyager visits a graph to number its neighborhoods. It proceeds gate by gate, doing 
 * the numbering recursively.
 */

#ifndef PIONEER_H_
#define PIONEER_H_

#include "Voyager.h"

class Pioneer : public Voyager
{
public:

    // Constructor, destructor
    Pioneer(Environment *env);
    ~Pioneer(); // Implicitely virtual
    
    void visit(Graph *g); // Implicitely virtual

protected:

    // Counter for the numbering
    unsigned int counter;
    
    // Method to recursively visit the graph, node by node.
    void visitRecursive(Neighborhood *n);

};

#endif /* PIONEER_H_ */
