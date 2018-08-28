/*
 * Voyager.h
 *
 *  Created on: Nov 16, 2017
 *      Author: jefgrailet
 *
 * This partially abstract class defines the interface for a bunch of classes which act as "graph 
 * travellers" (nicknamed voyagers) to perform various operations, such as visiting neighborhoods 
 * for alias resolution hint collection or simply numbering neighborhoods. They are equivalent to 
 * the "climber" classes in TreeNET.
 */

#ifndef VOYAGER_H_
#define VOYAGER_H_

#include "../../Environment.h"
#include "../Graph.h"

class Voyager
{
public:

    // Constructor, destructor
    Voyager(Environment *env);
    virtual ~Voyager();
    
    // To be implemented by children classes
    virtual void visit(Graph *g) = 0;

protected:

    // Unique field: environment object
    Environment *env;
    
};

#endif /* VOYAGER_H_ */
