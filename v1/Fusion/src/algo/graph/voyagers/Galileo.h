/*
 * Galileo.h
 *
 *  Created on: Nov 21, 2017
 *      Author: jefgrailet
 *
 * This voyager visits a graph to conduct alias resolution. In the process, it writes in streams 
 * all discovered aliases and fingerprints (per neighborhood) for output purposes.
 */

#ifndef GALILEO_H_
#define GALILEO_H_

#include "Voyager.h"
#include "../../aliasresolution/AliasHintCollector.h"
#include "../../aliasresolution/AliasResolver.h" // Also provides Router/Fingerprint classes

class Galileo : public Voyager
{
public:

    // Constructor, destructor
    Galileo(Environment *env);
    ~Galileo(); // Implicitely virtual
    
    void visit(Graph *g); // Implicitely virtual
    
    // Methods to output the aliases and fingerprints
    void figaro(string filename);
    void magnifico(string filename);

protected:

    // Alias resolution tools
    AliasHintCollector *ahc;
    AliasResolver *ar;

    // Streams to output aliases/fingerprints
    stringstream aliasesStream;
    stringstream fingerprintsStream;
    
    // Method to recursively visit the graph, node by node.
    void visitRecursive(Neighborhood *n);

};

#endif /* Galileo_H_ */
