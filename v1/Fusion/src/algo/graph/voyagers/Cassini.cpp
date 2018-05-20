/*
 * Cassini.cpp
 *
 *  Created on: Nov 29, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in Cassini.h (see this file to learn further about the goals of 
 * such a class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition

#include "Cassini.h"
#include "../components/Node.h"
#include "../components/Cluster.h"
#include "../components/DirectLink.h"
#include "../components/IndirectLink.h"
#include "../components/RemoteLink.h"

Cassini::Cassini(Environment *env) : Voyager(env)
{
    visited = NULL;
    nbToVisit = 0;
    
    this->reset();
}

Cassini::~Cassini()
{
    if(visited != NULL)
        delete[] visited;
}

void Cassini::reset()
{
    if(visited != NULL)
        delete[] visited;

    visited = NULL;
    nbToVisit = 0;
    
    reachableNodes.clear();
    visitedGates.clear();
    
    degreeAcc = 0.0;
    totalNodes = 0.0;
    
    maxInDegree = 0;
    maxOutDegree = 0;
    
    minTotDegree = 65535;
    maxTotDegree = 0;
    
    minNbSubnets = 65535;
    maxNbSubnets = 0;
    
    accSubnets = 0;
    oddSubnets = 0;
    shadSubnets = 0;
    credSubnets = 0;
    
    coveredIPs = 0;
    coveredIPsCred = 0;
    
    maxAliasAmount = 0;
    nbSingleAlias = 0;
    maxAliasSize = 0;
    totalAliases = 0.0;
    
    nbDirectLinks = 0;
    nbIndirectLinks = 0;
    nbRemoteLinks = 0;
    nbLinksWithMedium = 0;
}

void Cassini::visit(Graph *g)
{
    // Resets everything if something was set
    if(visited != NULL)
        this->reset();
    
    // Sets the "visited" array
    nbToVisit = g->getNbNeighborhoods();
    totalNodes = (float) nbToVisit;
    visited = new bool[nbToVisit];
    for(unsigned int i = 0; i < nbToVisit; ++i)
        visited[i] = false;
    
    // Visits the graph (node metrics)
    list<Neighborhood*> *gates = g->getGates();
    for(list<Neighborhood*>::iterator i = gates->begin(); i != gates->end(); ++i)
        this->visitRecursive1((*i));
    
    // For the second visit, a second array is used to now which nodes were visited last iteration
    bool *visitedPrevious = new bool[nbToVisit];
    for(unsigned int i = 0; i < nbToVisit; ++i)
    {
        visited[i] = false;
        visitedPrevious[i] = false;
    }
    
    // Visits the graph a second time (connected component metrics)
    for(list<Neighborhood*>::iterator i = gates->begin(); i != gates->end(); ++i)
    {
        if(visitedPrevious[(*i)->getID() - 1])
            continue;
        
        // Visits recursively, listing the gates of the component in the process
        list<unsigned int> componentGates;
        this->visitRecursive2((*i), &componentGates);
        
        // Counts the amount of visited nodes
        unsigned int nbNodes = 0;
        for(unsigned int j = 0; j < nbToVisit; ++j)
            if(visited[j] && !visitedPrevious[j])
                nbNodes++;
        
        reachableNodes.push_back(nbNodes);
        visitedGates.push_back(componentGates);
        
        // "Commits" all currently visited nodes in visitedPrevious
        for(unsigned int j = 0; j < nbToVisit; ++j)
            visitedPrevious[j] = visited[j];
    }
    
    delete[] visitedPrevious;
    visitedPrevious = NULL;
    
    // Cleans the "visited" array
    delete[] visited;
    visited = NULL;
    nbToVisit = 0;
}

void Cassini::visitRecursive1(Neighborhood *n)
{
    unsigned int ID = n->getID();
    if(visited[ID - 1])
        return;
    
    visited[ID - 1] = true;
    
    // Deals with each kind of degree, first "in" degree (#edges coming in)
    unsigned short inDegree = n->getInDegree();
    if(inDegree > maxInDegree)
    {
        maxInDegree = inDegree;
        maxInIDs.clear();
        maxInIDs.push_back(ID);
    }
    else if(inDegree == maxInDegree)
        maxInIDs.push_back(ID);
    
    // "Out" degree (#edges coming out)
    unsigned short outDegree = n->getOutDegree();
    if(outDegree > maxOutDegree)
    {
        maxOutDegree = outDegree;
        maxOutIDs.clear();
        maxOutIDs.push_back(ID);
    }
    else if(outDegree == maxOutDegree)
        maxOutIDs.push_back(ID);
    
    // Full degree (in + out)
    unsigned short totDegree = inDegree + outDegree;
    if(totDegree > maxTotDegree)
    {
        maxTotDegree = totDegree;
        maxTotIDs.clear();
        maxTotIDs.push_back(ID);
    }
    else if(totDegree == maxTotDegree)
        maxTotIDs.push_back(ID);
    if(totDegree < minTotDegree) // Amounts to "isolated islands" if = 0
    {
        minTotDegree = totDegree;
        minTotIDs.clear();
        minTotIDs.push_back(ID);
    }
    else if(totDegree == minTotDegree)
        minTotIDs.push_back(ID);
    
    degreeAcc += (float) inDegree;
    
    // Amount of subnets
    unsigned short nbSubnets = n->getNbSubnets();
    if(nbSubnets > maxNbSubnets)
    {
        maxNbSubnets = nbSubnets;
        maxNbSubIDs.clear();
        maxNbSubIDs.push_back(ID);
    }
    else if(nbSubnets == maxNbSubnets)
        maxNbSubIDs.push_back(ID);
    if(nbSubnets < minNbSubnets)
    {
        minNbSubnets = nbSubnets;
        minNbSubIDs.clear();
        minNbSubIDs.push_back(ID);
    }
    else if(nbSubnets == minNbSubnets)
        minNbSubIDs.push_back(ID);
    
    if(nbSubnets > 200)
    {
        if(Cluster* cluster = dynamic_cast<Cluster*>(n))
            superClusters.push_back(cluster->getID());
        else if(Node* node = dynamic_cast<Node*>(n))
            superNodes.push_back(node->getID());
    }
    
    // Subnet metrics
    unsigned int *metrics = new unsigned int[6];
    n->getSubnetMetrics(metrics);
    accSubnets += metrics[0];
    oddSubnets += metrics[1];
    shadSubnets += metrics[2];
    credSubnets += metrics[3];
    coveredIPs += metrics[4];
    coveredIPsCred += metrics[5];
    delete[] metrics;
    
    // Alias metrics
    if(n->canHaveAliases())
    {
        unsigned int curNbAlias = n->getNbAliases();
        if(curNbAlias > maxAliasAmount)
        {
            maxAliasAmount = curNbAlias;
            maxAmountIDs.clear();
            maxAmountIDs.push_back(ID);
        }
        else if(curNbAlias == maxAliasAmount)
            maxAmountIDs.push_back(ID);
        totalAliases += (float) curNbAlias;
        
        unsigned short curLargest = n->getLargestAliasSize();
        if(curLargest > maxAliasSize)
            maxAliasSize = curLargest;
        
        if(n->isOneAlias())
            nbSingleAlias++;
    }
    
    // Counting amount of direct/indirect/remote links via in edges
    list<Edge*> *prev = n->getOutEdges();
    for(list<Edge*>::iterator i = prev->begin(); i != prev->end(); ++i)
    {
        Edge *cur = (*i);
        if(cur->getType() == Edge::DIRECT_LINK)
        {
            nbDirectLinks++;
            nbLinksWithMedium++;
        }
        else if(cur->getType() == Edge::INDIRECT_LINK)
        {
            nbIndirectLinks++;
            if(((IndirectLink *) cur)->hasMedium())
                nbLinksWithMedium++;
        }
        else if(cur->getType() == Edge::REMOTE_LINK)
            nbRemoteLinks++;
    }
    
    // Recursion via out edges
    list<Edge*> *next = n->getOutEdges();
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
        this->visitRecursive1((*i)->getHead());
}

void Cassini::visitRecursive2(Neighborhood *n, list<unsigned int> *componentGates)
{
    unsigned int ID = n->getID();
    if(visited[ID - 1])
        return;
    
    visited[ID - 1] = true;
    
    // Recursion (in edges)
    list<Edge*> *prev = n->getInEdges();
    if(prev->size() > 0)
    {
        for(list<Edge*>::iterator i = prev->begin(); i != prev->end(); ++i)
            this->visitRecursive2((*i)->getTail(), componentGates);
    }
    // This is a gate
    else
        componentGates->push_back(ID);
    
    // Recursion (out edges)
    list<Edge*> *next = n->getOutEdges();
    for(list<Edge*>::iterator i = next->begin(); i != next->end(); ++i)
        this->visitRecursive2((*i)->getHead(), componentGates);
}

string Cassini::getMetrics()
{
    stringstream ss;
    
    ss << "Graph" << endl;
    ss << "-----" << endl;
    
    // In degree
    ss << "Maximum in degree: " << maxInDegree;
    if(maxInDegree > 1)
    {
        ss << " (";
        for(list<unsigned int>::iterator it = maxInIDs.begin(); it != maxInIDs.end(); ++it)
        {
            if(it != maxInIDs.begin())
                ss << ", ";
            ss << "N" << (*it);
        }
        ss << ")";
    }
    ss << endl;
    
    // Out degree
    ss << "Maximum out degree: " << maxOutDegree << " (";
    for(list<unsigned int>::iterator it = maxOutIDs.begin(); it != maxOutIDs.end(); ++it)
    {
        if(it != maxOutIDs.begin())
            ss << ", ";
        ss << "N" << (*it);
    }
    ss << ")" << endl;
    
    // Total degree
    ss << "Minimum total degree (in + out): " << minTotDegree;
    if(minTotIDs.size() > 1)
        ss << " (" << minTotIDs.size() << " nodes)\n";
    else
        ss << " (One node)\n";
    ss << "Maximum total degree (in + out): " << maxTotDegree << " (";
    for(list<unsigned int>::iterator it = maxTotIDs.begin(); it != maxTotIDs.end(); ++it)
    {
        if(it != maxTotIDs.begin())
            ss << ", ";
        ss << "N" << (*it);
    }
    ss << ")" << endl;
    
    // Average degree and graph density (N.B.: average total degree = 2 * in/out average degree)
    ss << "Average in/out degree: " << degreeAcc / totalNodes << endl;
    ss << "Graph density: " << degreeAcc / (totalNodes * totalNodes) << "\n" << endl;
    
    // Subnet metrics
    float totalTargetIPs = (float) env->getTotalIPsInitialTargets();
    float totalSubnets = (float) (accSubnets + oddSubnets + shadSubnets);
    float ratioAcc = ((float) accSubnets) / totalSubnets * 100;
    float ratioOdd = ((float) oddSubnets) / totalSubnets * 100;
    float ratioShad = ((float) shadSubnets) / totalSubnets * 100;
    float ratioCred = ((float) credSubnets) / totalSubnets * 100;
    float coverage = (float) coveredIPs / totalTargetIPs * 100;
    float coverageCred = (float) coveredIPsCred / totalTargetIPs * 100;
    float avgSubnets = totalSubnets / totalNodes;
    ss << "Subnets" << endl;
    ss << "-------" << endl;
    ss << "Accurate subnets: " << accSubnets << " (" << ratioAcc << "%)" << endl;
    ss << "Odd subnets: " << oddSubnets << " (" << ratioOdd << "%)" << endl;
    ss << "Shadow subnets: " << shadSubnets << " (" << ratioShad << "%)" << endl;
    ss << "Credible subnets: " << credSubnets << " (" << ratioCred << "%)" << endl;
    ss << "Covered IPs: " << coveredIPs << " (" << coverage << "% w.r.t. targets)" << endl;
    ss << "Covered IPs (credible subnets): " << coveredIPsCred << " (" << coverageCred << "% w.r.t. targets)" << endl;
    ss << "Minimum amount of subnets per neighborhood: " << minNbSubnets;
    if(minNbSubIDs.size() > 1)
        ss << " (" << minNbSubIDs.size() << " nodes)\n";
    else
        ss << " (One node)\n";
    ss << "Maximum amount of subnets per neighborhood: " << maxNbSubnets << " (";
    for(list<unsigned int>::iterator it = maxNbSubIDs.begin(); it != maxNbSubIDs.end(); ++it)
    {
        if(it != maxNbSubIDs.begin())
            ss << ", ";
        ss << "N" << (*it);
    }
    ss << ")" << endl;
    ss << "Average amount of subnets per neighborhood: " << avgSubnets << "\n" << endl;
    
    // Alias metrics
    float ratioSingle = ((float) nbSingleAlias) / totalNodes * 100;
    float avgAliases = totalAliases / totalNodes;
    ss << "Aliases" << endl;
    ss << "-------" << endl;
    ss << "Fully aliased neighborhoods: " << nbSingleAlias << " (" << ratioSingle << "%)" << endl;
    ss << "Maximum size of an alias: " << maxAliasSize << endl;
    ss << "Maximum amount of aliases: " << maxAliasAmount << " (";
    for(list<unsigned int>::iterator it = maxAmountIDs.begin(); it != maxAmountIDs.end(); ++it)
    {
        if(it != maxAmountIDs.begin())
            ss << ", ";
        ss << "N" << (*it);
    }
    ss << ")" << endl;
    ss << "Average amount of aliases: " << avgAliases << "\n" << endl;
    
    // Quantities/ratios for each kind of links
    float totalLinks = (float) (nbDirectLinks + nbIndirectLinks + nbRemoteLinks);
    if(totalLinks > 0)
    {
        float ratioDirect = ((float) nbDirectLinks) / totalLinks * 100;
        float ratioIndirect = ((float) nbIndirectLinks) / totalLinks * 100;
        float ratioRemote = ((float) nbRemoteLinks) / totalLinks * 100;
        float ratioWithMedium = ((float) nbLinksWithMedium) / totalLinks * 100;
        ss << "Links" << endl;
        ss << "-----" << endl;
        ss << "Direct links: " << nbDirectLinks << " (" << ratioDirect << "%)" << endl;
        ss << "Indirect links: " << nbIndirectLinks << " (" << ratioIndirect << "%)" << endl;
        ss << "Remote links: " << nbRemoteLinks << " (" << ratioRemote << "%)" << endl;
        ss << "Links with a medium: " << nbLinksWithMedium << " (" << ratioWithMedium << "%)\n" << endl;
    }
    
    // Reachable nodes per gate + islets
    ss << "Connected components" << endl;
    ss << "--------------------" << endl;
    stringstream islets;
    unsigned int nbIslets = 0;
    list<list<unsigned int> >::iterator it = visitedGates.begin();
    for(list<unsigned int>::iterator i = reachableNodes.begin(); i != reachableNodes.end(); ++i)
    {
        // Gets the total amount of visited nodes
        unsigned int nbReachable = (*i);
    
        // Gets the visited gates of the component
        list<unsigned int> visitedGates = (*it);
        it++;
        
        // Islet scenario
        if(nbReachable == 1)
        {
            if(nbIslets > 0)
            {
                islets << ", ";
                if((nbIslets % 10) == 0)
                    islets << "\n";
            }
            islets << "N" << visitedGates.front();
            nbIslets++;
            continue;
        }
        
        // Component with more than one node, possibly with multiple gates
        float percentage = ((float) nbReachable) / totalNodes * 100;
        unsigned int nbVisitedGates = visitedGates.size();
        if(nbVisitedGates > 1)
        {
            stringstream gatesStream;
            for(list<unsigned int>::iterator j = visitedGates.begin(); j != visitedGates.end(); ++j)
            {
                if(j != visitedGates.begin())
                    gatesStream << ", ";
                gatesStream << "N" << (*j);
            }
            ss << "Via gates " << gatesStream.str() << ": ";
            ss << nbReachable << " (" << percentage << "%)" << endl;
        }
        else
        {
            ss << "Via gate N" << visitedGates.front() << ": ";
            ss << nbReachable << " (" << percentage << "%)" << endl;
        }
    }
    
    if(nbIslets > 0)
    {
        float ratioIslets = (float) nbIslets / totalNodes * 100;
        if(nbIslets == 1)
        {
            ss << "Amount of islets: " << nbIslets << " (" << ratioIslets << "%; ";
            ss << islets.str() << ")" << endl;
        }
        else
        {
            ss << "Amount of islets: " << nbIslets << " (" << ratioIslets << "%)" << endl;
            ss << "Detailed list: " << islets.str() << endl;
        }
    }
    
    if(superNodes.size() > 0 || superClusters.size() > 0)
    {
        ss << "\n";
        ss << "Exceptional neighborhoods\n";
        ss << "-------------------------" << endl;
        
        unsigned int totalSuper = superNodes.size() + superClusters.size();
        float ratioNodes = ((float) superNodes.size() / (float) totalSuper) * 100;
        float ratioClusters = ((float) superClusters.size() / (float) totalSuper) * 100;
        ss << "Super neighborhoods (> 200 subnets): " << totalSuper << endl;
        
        stringstream detailedNodes, detailedClusters;
        if(superNodes.size() > 0)
        {
            unsigned int nbNodes = 0;
            for(list<unsigned int>::iterator i = superNodes.begin(); i != superNodes.end(); ++i)
            {
                if(nbNodes > 0)
                {
                    detailedNodes << ", ";
                    if((nbNodes % 10) == 0)
                        detailedNodes << "\n";
                }
                detailedNodes << "N" << (*i);
                nbNodes++;
            }
        }
        if(superClusters.size() > 0)
        {
            unsigned int nbClusters = 0;
            for(list<unsigned int>::iterator i = superClusters.begin(); i != superClusters.end(); ++i)
            {
                if(nbClusters > 0)
                {
                    detailedClusters << ", ";
                    if((nbClusters % 10) == 0)
                        detailedClusters << "\n";
                }
                detailedClusters << "N" << (*i);
                nbClusters++;
            }
        }
        
        if(superNodes.size() > 0 && superClusters.size() > 0)
        {
            ss << "Super nodes: " << superNodes.size() << " (" << ratioNodes << "%)" << endl;
            ss << "Super clusters: " << superClusters.size() << " (" << ratioClusters << "%)" << endl;
            ss << "Super node list: " << detailedNodes.str() << endl;
            ss << "Super cluster list: " << detailedClusters.str() << endl;
        }
        else if(superNodes.size() > 0)
        {
            ss << "There are only super nodes." << endl;
            ss << "Super node list: " << detailedNodes.str() << endl;
        }
        else
        {
            ss << "There are only super clusters." << endl;
            ss << "Super cluster list: " << detailedClusters.str() << endl;
        }
    }
    
    return ss.str();
}

void Cassini::outputMetrics(string filename)
{
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << this->getMetrics();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}
