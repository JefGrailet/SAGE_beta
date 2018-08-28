/*
 * Mariner.cpp
 *
 *  Created on: Nov 16, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Mariner.h (see this file to learn further about the goals of 
 * such a class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition

#include "Mariner.h"
#include "../components/Cluster.h"
#include "../components/Node.h"

Mariner::Mariner(Environment *env) : Voyager(env)
{
    // Following fields are only set (and cleaned/deleted) in visit()
    visited = NULL;
    nbToVisit = 0;
}

Mariner::~Mariner()
{
    nodes.clear();
}

void Mariner::visit(Graph *g)
{
    // Sets the "visited" array
    nbToVisit = g->getNbNeighborhoods();
    visited = new bool[nbToVisit];
    for(unsigned int i = 0; i < nbToVisit; ++i)
        visited[i] = false;
    
    // Visits the graph
    list<Neighborhood*> *gates = g->getGates();
    for(list<Neighborhood*>::iterator i = gates->begin(); i != gates->end(); ++i)
        this->visitRecursive((*i));
    nodes.sort(Neighborhood::smallerID);
    
    // Gets all miscellaneous nodes
    misc = g->getMiscHops();
    
    // Cleans the "visited" array
    delete[] visited;
    visited = NULL;
    nbToVisit = 0;
}

void Mariner::visitRecursive(Neighborhood *n)
{
    unsigned int ID = n->getID();
    if(visited[ID - 1])
        return;
    
    nodes.push_back(n);
    visited[ID - 1] = true;
    
    // Recursion
    list<Edge*> *next = n->getOutEdges();
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
        this->visitRecursive((*i)->getHead());
}

void Mariner::outputNeighborhoods(string filename)
{
    string output = "";
    for(list<Neighborhood*>::iterator i = nodes.begin(); i != nodes.end(); ++i)
    {
        Neighborhood *n = (*i);
        string cur = n->toString();
        
        if(!cur.empty())
            output += cur + "\n";
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void Mariner::outputGraph(string filename)
{
    stringstream ss;
    
    // First, list of neighborhoods (mapping ID -> label)
    for(list<Neighborhood*>::iterator i = nodes.begin(); i != nodes.end(); ++i)
    {
        Neighborhood *n = (*i);
        ss << "N" << n->getID() << " - " << n->getFullLabel();
        
        // Checks that the label(s) was (were) among the targets for subnet inference
        if(Cluster* cluster = dynamic_cast<Cluster*>(n))
        {
            list<InetAddress> *labels = cluster->getLabels();
            stringstream subStream;
            bool guardian = false;
            for(list<InetAddress>::iterator j = labels->begin(); j != labels->end(); ++j)
            {
                if(!env->initialTargetsEncompass((*j)))
                {
                    if(!guardian)
                        guardian = true;
                    else
                        subStream << ", ";
                    subStream << (*j);
                }
            }
            string result = subStream.str();
            if(result.length() > 0)
                ss << " (" << result << " not among targets)";
        }
        else if(Node* node = dynamic_cast<Node*>(n))
        {
            InetAddress label = node->getLabel();
            if(!env->initialTargetsEncompass(label))
                ss << " (not among targets)";
        }
        
        ss << "\n";
    }
    ss << "\n";
    
    // Then, the "out" edges for each neighborhood
    for(list<Neighborhood*>::iterator i = nodes.begin(); i != nodes.end(); ++i)
    {
        Neighborhood *n = (*i);
        list<Edge*> *edges = n->getOutEdges();
        
        if(edges->size() == 0)
            continue;
        
        for(list<Edge*>::iterator j = edges->begin(); j != edges->end(); ++j)
        {
            Edge *e = (*j);
            ss << e->toString() << "\n";
        }
    }
    
    // Finally, the miscellaneous hops (intermediate for remote links) if they exist
    if(misc.size() > 0)
    {
        ss << "\n";
        for(list<MiscHop*>::iterator i = misc.begin(); i != misc.end(); ++i)
            ss << (*i)->toString();
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << ss.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void Mariner::cleanNeighborhoods()
{
    for(list<Neighborhood*>::iterator i = nodes.begin(); i != nodes.end(); ++i)
    {
        delete (*i);
    }
    nodes.clear();
}
