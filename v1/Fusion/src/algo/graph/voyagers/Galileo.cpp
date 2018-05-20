/*
 * Galileo.cpp
 *
 *  Created on: Nov 21, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Galileo.h (see this file to learn further about the goals of 
 * such a class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition

#include "../../../common/thread/Thread.h" // For invokeSleep()
#include "Galileo.h"

Galileo::Galileo(Environment *env) : Voyager(env)
{
    ahc = new AliasHintCollector(env);
    ar = new AliasResolver(env);
}

Galileo::~Galileo()
{
    delete ahc;
    delete ar;
}

void Galileo::visit(Graph *g)
{
    list<Neighborhood*> *gates = g->getGates();
    for(list<Neighborhood*>::iterator i = gates->begin(); i != gates->end(); ++i)
        this->visitRecursive((*i));
}

void Galileo::visitRecursive(Neighborhood *n)
{
    ostream *out = env->getOutputStream();

    // Does not visit if the node already has inferred routers
    if(n->getAliases()->size() > 0)
        return;
    
    (*out) << "Conducting alias resolution on neighborhood " << n->getFullLabel();
    (*out) << "... " << std::flush;
    
    if(this->ahc->isPrintingSteps())
    {
        (*out) << endl;
        if(this->ahc->debugMode()) // Additionnal line break for harmonious display
            (*out) << endl;
    }
    
    ahc->collect(n);
    ar->resolve(n);
    
    // Gets the results
    list<Router*> *aliases = n->getAliases();
    list<Fingerprint> *fingerprints = n->getFingerprints();
    
    // Writes them
    if(aliases->size() > 0)
    {
        aliasesStream << "N" << n->getID() << " - " << n->getFullLabel() << ":\n";
        for(list<Router*>::iterator i = aliases->begin(); i != aliases->end(); ++i)
            aliasesStream << (*i)->toStringSemiVerbose() << "\n";
        aliasesStream << "\n";
    }
    
    if(fingerprints->size() > 0)
    {
        fingerprintsStream << "N" << n->getID() << " - " << n->getFullLabel() << ":\n";
        for(list<Fingerprint>::iterator i = fingerprints->begin(); i != fingerprints->end(); ++i)
            fingerprintsStream << (InetAddress) (*((*i).ipEntry)) << " - " << (*i) << "\n";
        fingerprintsStream << "\n";
    }
    
    // Small delay before analyzing next neighborhood (typically quarter of a second)
    Thread::invokeSleep(env->getProbeThreadDelay());
    
    // Recursion
    list<Edge*> *next = n->getOutEdges();
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
        this->visitRecursive((*i)->getHead());
}

void Galileo::figaro(string filename)
{
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << aliasesStream.str() << std::flush;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void Galileo::magnifico(string filename)
{
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << fingerprintsStream.str() << std::flush;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
