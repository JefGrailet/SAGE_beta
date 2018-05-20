/*
 * NetworkScanner.h
 *
 *  Created on: Nov 14, 2016
 *      Author: jefgrailet
 *
 * This class was added to move a large part of Main.cpp away and make it more readable. Indeed, 
 * the code scheduling the scanning phase was wholy written in Main.cpp, while other steps are 
 * dealt with with calls to specific classes in the main code. Re-factoring the code in this 
 * manner helps to keep the overall architecture easy to understand and the main code easy to read.
 */

#ifndef NETWORKSCANNER_H_
#define NETWORKSCANNER_H_

#include "../Environment.h"
#include "./refinement/SubnetRefiner.h"

#include <list>
using std::list;

class NetworkScanner
{
public:

    // Constructor, destructor
    NetworkScanner(Environment *env);
    ~NetworkScanner();
    
    // Two methods: one for the scanning itself, another for final steps (e.g. shadow expansion)
    void scan(list<InetAddress> targets);
    void finalize();
    
private:

    // Pointer to the environment singleton (provides access to the subnet set and prober parameters)
    Environment *env;
    
    // SubnetRefiner instance (can be the same for the whole execution)
    SubnetRefiner *sr;

}; 

#endif /* NETWORKSCANNER_H_ */
