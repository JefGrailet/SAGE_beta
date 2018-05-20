/*
 * GraphBuilder.cpp
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in GraphBuilder.h (see this file to learn further about the goals 
 * of such a class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition
#include <map>
using std::map;

#include "GraphBuilder.h"
#include "components/Node.h"
#include "components/DirectLink.h"
#include "components/IndirectLink.h"
#include "components/RemoteLink.h"
#include "voyagers/Pioneer.h"

GraphBuilder::GraphBuilder(Environment *env)
{
    this->env = env;
    ahc = new AliasHintCollector(env);
    ar = new AliasResolver(env);
    result = NULL;
}

GraphBuilder::~GraphBuilder()
{
    delete ahc;
    delete ar;
}

Graph* GraphBuilder::getResult()
{
    Graph *res = result;
    result = NULL;
    return res;
}

/*
 * Note: normally I would put the next 2 methods below build(), but due to build() length and the 
 * related private methods implemented after, it's more readable to have these short methods 
 * before.
 */

bool GraphBuilder::gotAnomalies()
{
    if(anomalies.gcount() > 0)
        return true;
    return false;
}

void GraphBuilder::outputAnomalies(string filename)
{
    if(anomalies.gcount() == 0)
        return;
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << anomalies.str();
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

void GraphBuilder::build()
{
    ostream *out = env->getOutputStream();
    if(env->getSubnetSet()->getNbSubnets() == 0)
    {
        (*out) << "No subnet is available. No graph will be built." << endl;
        return;
    }
    
    /*
     * Structures to handle subnet aggregates:
     * -subnetsByLabel: a map to build aggregates. Subnets are mapped to their neighborhood 
     *  label, as a string. Aggregates can then be created by iterating the content (lists of 
     *  subnets) of the map.
     * -aggsByIP: a map containing aggregates organized according to their respective neighborhood 
     *  label and used during step 3 to build junction neighborhoods. When several aggregates have 
     *  the same label IP, only the aggregate associated to the label with the smallest length 
     *  (ideally zero) is inserted in the map.
     * -aggregates: a list simply containing all aggregates. The pointers it contain are the same 
     *  as in the map "aggsByIP".
     */
    
    map<string, list<SubnetSite*>* > subnetsByLabel;
    map<InetAddress, Aggregate*> aggsByIP;
    list<Aggregate*> aggregates;
    
    /*
     * Step 1: subnet aggregation
     * --------------------------
     * Using the subnetsByLabel map, this part of the code builds the aggregates of subnets that 
     * will later constitutes neighborhood nodes, either as simple Node objects, either as Cluster 
     * objects (made of several aggregates).
     */
    
    env->prepareForGraphBuilding();
    
    // Fills subnetsByLabel
    list<SubnetSite*> *ssList = env->getSubnetSet()->getSubnetSiteList();
    while(ssList->size() > 0)
    {
        SubnetSite *ss = ssList->front();
        ssList->pop_front();
        
        // Gets rid of subnets without a route (at this stage, they should still be in .subnets)
        if(!ss->hasValidRoute())
        {
            delete ss;
            continue;
        }
        
        string ssLabel = ss->getNeighborhoodLabelString();
        map<string, list<SubnetSite*>* >::iterator res = subnetsByLabel.find(ssLabel);
        if(res != subnetsByLabel.end())
        {
            list<SubnetSite*> *existingList = res->second;
            existingList->push_back(ss);
        }
        else
        {
            list<SubnetSite*> *newList = new list<SubnetSite*>();
            newList->push_back(ss);
            
            subnetsByLabel.insert(pair<string, list<SubnetSite*>* >(ssLabel, newList));
        }
    }
    
    // Iterates the map to build the aggregates
    map<string, list<SubnetSite*>* >::iterator it;
    for(it = subnetsByLabel.begin(); it != subnetsByLabel.end(); ++it)
    {
        list<SubnetSite*> *toAggregate = it->second;
        toAggregate->sort(SubnetSite::compare);
        
        Aggregate *newAgg = new Aggregate((*toAggregate));
        aggregates.push_back(newAgg);
        
        /*
         * Because of routing issues (like cycling), it's possible that an aggregate with a label 
         * length above 0 can still be the peer of other aggregates. To take that into account, 
         * the map allows mappings between an IP and such an aggregate, as long as they is no 
         * other aggregate with a smaller label length (ideally zero). In practice, before 
         * inserting an <IP, aggregate> pair, the code checks if such a pair does not already 
         * exist. If yes, it replaces the pair if and only if the label length is strictly 
         * smaller, otherwise the known aggregate is kept.
         */
        
        InetAddress labelIP = newAgg->getLabelIP();
        map<InetAddress, Aggregate*>::iterator match = aggsByIP.find(labelIP);
        if(match != aggsByIP.end())
        {
            Aggregate *knownAgg = match->second;
            if(newAgg->getLabelAnomalies() < knownAgg->getLabelAnomalies()) // Must be strictly inferior
            {
                aggsByIP.erase(match);
                aggsByIP.insert(pair<InetAddress, Aggregate*>(newAgg->getLabelIP(), newAgg));
            }
        }
        else
            aggsByIP.insert(pair<InetAddress, Aggregate*>(newAgg->getLabelIP(), newAgg));
        
        delete toAggregate;
    }
    
    /*
     * Step 2: peer estimation
     * -----------------------
     * For each aggregate and its associated neighborhood label, the code estimates what are the 
     * "peers" for the IP of the label - i.e., the closest IP(s) of the label of other aggregates 
     * that are crossed by the routes towards the subnets of the current aggregate. When several 
     * possibilities exist at a same hop count, the code (through private method identifyPeers()) 
     * conducts alias resolution to ensure the different IPs belong to a same device (therefore, a 
     * same peer) or distinct devices.
     */
    
    for(list<Aggregate*>::iterator it = aggregates.begin(); it != aggregates.end(); ++it)
    {
        Aggregate *curAgg = (*it);
        (*out) << curAgg->toString() << endl;
        
        if(this->isConnectionNeeded(curAgg))
            this->estimatePeers(curAgg);
        else
            (*out) << "This aggregate should be a gate.\n" << endl;
    }
    
    /*
     * Step 3: peer aggregation
     * ------------------------
     * Peer estimation alone is not enough to ensure accuracy of peers/pre-aliases. It is indeed 
     * possible two separate aggregates share one previous route interface (before neighborhood 
     * label) found in an alias and end up with two different aliases lists containing this IP. 
     * Ideally, both aggregates should have the same peer with the same list of IPs.
     *
     * An iterative algorithm involving several maps (described in details in subsequent comments) 
     * is used to ensure all Peer objects sharing common IPs have the same exact list of IP 
     * interfaces.
     *
     * It is also during this step that the actual Graph object is created, because the first 
     * Neighborhood objects created in the process are used to create the first subnet map entries 
     * of the graph (remaining entries will be created at the start of step 4), and gates of the 
     * graph can actually be already pointed out at this point (= neighborhoods without peers).
     */
    
    result = new Graph();
    
    /*
     * The aggregation starts here. It consists in repeating the following steps:
     * 1) Creating a map IP -> list of peers.
     * 2) For each list of peers in the map, 
     *    A) Creates another map giving the IPs different from the IP associated with the current 
     *       list, for each peer.
     *    B) If the size of this additionnal map is 0 or if the size of each list within the map 
     *       equals the size of the current list of peers, then no aggregation is needed.
     *    C) Otherwise, the final list of IPs for each peer is created and peers are updated in 
     *       that regard.
     * 3) Re-start the whole process as long as the algorithm founds IPs missing from at least one 
     *    peer during step 2.B) (any list of peers).
     */
    
    bool newIPs;
    do
    {
        newIPs = false;
        
        // 1) Builds a map with the peers (IP -> list of peers featuring that IP).
        map<InetAddress,list<Peer*>*> peersMap;
        for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
        {
            Aggregate *curAgg = (*i);
            list<Peer*> *curPeers = curAgg->getPeers();
            for(list<Peer*>::iterator j = curPeers->begin(); j != curPeers->end(); ++j)
            {
                Peer *curPeer = (*j);
                list<InetAddress> *interfaces = curPeer->getInterfaces();
                list<InetAddress>::iterator k;
                for(k = interfaces->begin(); k != interfaces->end(); ++k)
                {
                    InetAddress IP = (*k);
                    
                    list<Peer*> *peersList = NULL;
                    map<InetAddress,list<Peer*>*>::iterator res = peersMap.find(IP);
                    if(res != peersMap.end())
                    {
                        peersList = res->second;
                    }
                    else
                    {
                        peersList = new list<Peer*>();
                        peersMap.insert(pair<InetAddress,list<Peer*>*>(IP, peersList));
                    }
                    peersList->push_back(curPeer);
                }
            }
        }
        
        // 2) For each list of peers in the map...
        map<InetAddress,list<Peer*>*>::iterator it;
        for(it = peersMap.begin(); it != peersMap.end(); ++it)
        {
            InetAddress curIP = it->first;
            list<Peer*> *peers = it->second;
            
            /*
             * A) For each peer, maps it to IPs that are different from cur, via an additionnal 
             * C/C++ map (IP -> list of peers with that IP with IP =/= curIP).
             */
            
            map<InetAddress, list<Peer*>*> differingIPs;
            for(list<Peer*>::iterator i = peers->begin(); i != peers->end(); ++i)
            {
                Peer *curPeer = (*i);
                list<InetAddress> *peerIPs = curPeer->getInterfaces();
                for(list<InetAddress>::iterator j = peerIPs->begin(); j != peerIPs->end(); ++j)
                {
                    if((*j) == curIP)
                        continue;
                    
                    InetAddress newIP = (*j);
                    list<Peer*> *peersList = NULL;
                    map<InetAddress,list<Peer*>*>::iterator res = differingIPs.find(newIP);
                    if(res != differingIPs.end())
                    {
                        peersList = res->second;
                    }
                    else
                    {
                        peersList = new list<Peer*>();
                        differingIPs.insert(pair<InetAddress,list<Peer*>*>(newIP, peersList));
                    }
                    peersList->push_back(curPeer);
                }
            }
            
            /*
             * B) Checks if aggregation is required. If the size of the map is zero OR the size 
             * of each list in differingIPs is equal to the amount of peers of the current peers 
             * list, then there is no need to aggregate anything.
             */
            
            if(differingIPs.size() == 0)
            {
                map<InetAddress,list<Peer*>*>::iterator i;
                for(i = differingIPs.begin(); i != differingIPs.end(); ++i)
                    delete i->second;
                continue;
            }
            else
            {
                bool needsAggregation = false;
                map<InetAddress,list<Peer*>*>::iterator i;
                for(i = differingIPs.begin(); i != differingIPs.end(); ++i)
                {
                    if(i->second->size() < peers->size())
                    {
                        needsAggregation = true;
                        break;
                    }
                }
                
                if(needsAggregation)
                {
                    newIPs = true;
                }
                else
                {
                    for(i = differingIPs.begin(); i != differingIPs.end(); ++i)
                        delete i->second;
                    continue;
                }
            }
            
            /*
             * C) If we reach this point, then the lists of IPs of each peer need to be updated. 
             * A list of all IPs is computed and each peer is updated with it.
             */
            
            map<InetAddress,list<Peer*>*>::iterator i;
            list<InetAddress> finalIPs;
            finalIPs.push_back(curIP);
            for(i = differingIPs.begin(); i != differingIPs.end(); ++i)
                finalIPs.push_back(i->first);
            finalIPs.sort(InetAddress::smaller);
            
            for(list<Peer*>::iterator i = peers->begin(); i != peers->end(); ++i)
            {
                Peer *curPeer = (*i);
                list<InetAddress> *peerIPs = curPeer->getInterfaces();
                
                peerIPs->clear();
                peerIPs->insert(peerIPs->begin(), finalIPs.begin(), finalIPs.end());
            }
            
            /*
             * Deletes the current differenIPs map.
             */
            
            for(i = differingIPs.begin(); i != differingIPs.end(); ++i)
                delete i->second;
        }
        
        // 3) Deletes the contents of peersMap and re-start if needed (newIPs = true).
        for(it = peersMap.begin(); it != peersMap.end(); ++it)
            delete it->second;
    }
    while(newIPs);
    
    // Post-processes the peers of each aggregate to avoid any duplicate
    list<Peer*> peers;
    for(list<Aggregate*>::iterator it = aggregates.begin(); it != aggregates.end(); ++it)
    {
        Aggregate *curAgg = (*it);
        this->postProcessPeers(curAgg);
    }
    
    // Creates a map of peers per string equivalent
    map<string, list<Peer*>*> peersPerStr;
    for(list<Aggregate*>::iterator i = aggregates.begin(); i != aggregates.end(); ++i)
    {
        Aggregate *curAgg = (*i);
        list<Peer*> *curPeers = curAgg->getPeers();
        for(list<Peer*>::iterator j = curPeers->begin(); j != curPeers->end(); ++j)
        {
            Peer *curPeer = (*j);
            string curPeerStr = curPeer->toString(false);
            list<Peer*> *peersList = NULL;
            map<string,list<Peer*>*>::iterator res = peersPerStr.find(curPeerStr);
            if(res != peersPerStr.end())
            {
                peersList = res->second;
            }
            else
            {
                peersList = new list<Peer*>();
                peersPerStr.insert(pair<string, list<Peer*>*>(curPeerStr, peersList));
            }
            peersList->push_back(curPeer);
        }
    }
    
    list<Cluster*> clusters; // For some post-processing
    map<string, list<Peer*>*>::iterator it2;
    for(it2 = peersPerStr.begin(); it2 != peersPerStr.end(); ++it2)
    {
        list<Peer*> *curList = it2->second;
        
        // Creates the concrete peer
        Neighborhood *concretePeer = NULL;
        list<InetAddress> *IPs = curList->front()->getInterfaces();
        if(IPs->size() > 1)
        {
            this->registerPreAlias(curList->front());
        
            list<Aggregate*> aggs;
            list<InetAddress> collateral; // Aliased IPs that are not junction IPs
            for(list<InetAddress>::iterator it = IPs->begin(); it != IPs->end(); ++it)
            {
                map<InetAddress, Aggregate*>::iterator res = aggsByIP.find((*it));
                if(res != aggsByIP.end()) // No res can occur if an aliased IP is not a "junction"
                {
                    Aggregate *resAgg = res->second;
                    
                    aggs.push_back(resAgg);
                    resAgg->markAsJunction();
                }
                else
                    collateral.push_back((*it));
            }
            
            // Defensive programming
            if(aggs.size() == 0)
            {
                string peerStr = curList->front()->toString();
                
                // Logs the anomaly
                anomalies << "Neighborhood instantiation error: there is no aggregate matching ";
                anomalies << "an IP in " << peerStr << "." << endl;
                continue;
            }
            
            Cluster *newCluster = new Cluster(aggs); // List is necessarily non-empty
            clusters.push_back(newCluster);
            concretePeer = (Neighborhood*) newCluster;
            
            // Adds other aliased IPs that aren't junction IPs
            if(collateral.size() > 0)
                for(list<InetAddress>::iterator it = collateral.begin(); it != collateral.end(); ++it)
                    ((Cluster*) concretePeer)->addLabel((*it));
        }
        else
        {
            map<InetAddress, Aggregate*>::iterator res = aggsByIP.find(IPs->front());
            // Defensive programming
            if(res == aggsByIP.end())
            {
                // Logs the anomaly
                anomalies << "Neighborhood instantiation error: there is no aggregate matching ";
                anomalies << "the IP for peer " << curList->front()->toString() << "." << endl;
                continue;
            }
            Aggregate *resAgg = res->second;
            
            concretePeer = new Node(resAgg);
            resAgg->markAsJunction();
        }
        
        result->mapSubnetsFrom(concretePeer);
        if(concretePeer->getNbPeers() == 0)
            result->addGate(concretePeer);
        
        // Associates the concrete peer to each Peer object
        for(list<Peer*>::iterator it = curList->begin(); it != curList->end(); ++it)
            (*it)->setConcretePeer(concretePeer);
    }
    
    for(list<Cluster*>::iterator i = clusters.begin(); i != clusters.end(); ++i)
        this->postProcessPeers((*i));
    
    for(it2 = peersPerStr.begin(); it2 != peersPerStr.end(); ++it2)
        delete it2->second;
    
    /*
     * Step 4: edge creation
     * ---------------------
     * Starting with the "termini" neighborhoods (built using aggregates which the "junction" flag 
     * is not raised), neighborhoods are progressively connected together. Each time an edge is 
     * created, the corresponding "Peer" object is removed from the list of peers of the 
     * neighborhood (this both frees memory no longer needed and avoids creating duplicate links). 
     * As each neighborhood is annotated with the edges coming in/out, a gate is identified as a 
     * neighborhood with no "in" edge.
     */
    
    // Lists "termini" neighborhoods (0(N))
    list<Aggregate*> termini;
    for(list<Aggregate*>::iterator it = aggregates.begin(); it != aggregates.end(); ++it)
    {
        Aggregate *curAgg = (*it);
        if(!curAgg->isAJunction())
        {
            termini.push_back(curAgg);
            (*out) << "Terminus: " << curAgg->getLabelString() << endl;
        }
    }
    
    // Creates the "termini" neighborhoods and recursively creates edges with each.
    for(list<Aggregate*>::iterator it = termini.begin(); it != termini.end(); ++it)
    {
        Neighborhood *newNode = new Node((*it));
        result->mapSubnetsFrom(newNode);
        if(newNode->getNbPeers() == 0)
        {
            result->addGate(newNode);
            (*out) << "Gate: " << newNode->getFullLabel() << endl;
        }
        else
            this->connectToPeers(newNode);
    }
    
    // Now, we can free Aggregate objects. At this point, there is no more Peer/Aggregate objects.
    for(list<Aggregate*>::iterator it = aggregates.begin(); it != aggregates.end(); ++it)
        delete (*it);
    
    /*
     * Step 5: neighborhood numbering
     * ------------------------------
     * The final step of the graph building simply consists in numbering the neighborhoods, 
     * starting with the gates, such that a convenient output file can be written afterwards: it's 
     * more economic to denote each neighborhood with a notation "N[number]" at first, then write 
     * the links using this notation. It's also more readable with Cluster neighborhoods.
     */ 
    
    Voyager *pioneer = new Pioneer(env);
    pioneer->visit(result);
    delete pioneer;
}

bool GraphBuilder::isConnectionNeeded(Aggregate *a)
{
    list<SubnetSite*> *subnets = a->getSubnets();
    unsigned short labelAnomalies = subnets->front()->getNeighborhoodLabelAnomalies();
    for(list<SubnetSite*>::iterator it = subnets->begin(); it != subnets->end(); ++it)
    {
        unsigned short routeSize = (*it)->getFinalRouteSize();
        RouteInterface *route = (*it)->getRoute();
        for(unsigned short i = 0; i < routeSize - labelAnomalies - 1; ++i)
            if(route[i].neighborhoodLabel)
                return true;
    }
    return false;
}

void GraphBuilder::estimatePeers(Aggregate *a)
{
    list<SubnetSite*> *subnets = a->getSubnets();
    list<Peer*> *peers = a->getPeers();
    list<SubnetSite*> listCopy((*subnets)), subList;
    
    // Builds a mapping <route length, subnets>
    map<unsigned short, list<SubnetSite*>* > perRouteLen;
    for(list<SubnetSite*>::iterator it = subnets->begin(); it != subnets->end(); ++it)
    {
        unsigned short routeLen = (*it)->getFinalRouteSize();
        map<unsigned short, list<SubnetSite*> *>::iterator res = perRouteLen.find(routeLen);
        if(res != perRouteLen.end())
        {
            res->second->push_back((*it));
        }
        else
        {
            list<SubnetSite*> *newList = new list<SubnetSite*>();
            newList->push_back((*it));
            perRouteLen.insert(pair<unsigned short, list<SubnetSite*>* >(routeLen, newList));
        }
    }
    
    // For each list, discovers peers
    map<unsigned short, list<SubnetSite*> *>::iterator it;
    for(it = perRouteLen.begin(); it != perRouteLen.end(); ++it)
    {
        list<Peer*> newPeers = this->identifyPeers(*(it->second));
        peers->insert(peers->end(), newPeers.begin(), newPeers.end());
        delete it->second;
    }
    
    this->postProcessPeers(a);
    
    // Display
    ostream *out = env->getOutputStream();
    if(peers->size() > 1)
    {
        (*out) << "Found " << peers->size() << " peers for " << a->getLabelString() << ":\n";
        for(list<Peer*>::iterator it = peers->begin(); it != peers->end(); ++it)
            (*out) << (*it)->toString() << "\n";
        (*out) << endl;
    }
    else if(peers->size() == 1)
    {
        (*out) << "Found one peer for " << a->getLabelString();
        (*out) << ": " << peers->front()->toString() << ".\n" << endl;
    }
}

list<Peer*> GraphBuilder::identifyPeers(list<SubnetSite*> subnets)
{
    IPLookUpTable *dict = env->getIPTable();
    list<Peer*> result;
    
    // 1) Find at which distance from the neighborhood label the first junction(s) appear(s)
    unsigned short offset = 1;
    unsigned short routeLength = subnets.front()->getFinalRouteSize();
    unsigned short labelAnomalies = subnets.front()->getNeighborhoodLabelAnomalies();
    InetAddress labelIP = subnets.front()->getNeighborhoodLabelIP();
    if(labelAnomalies > 0)
        routeLength -= labelAnomalies; // Due to anomalies at the end, the route length is shortened
    
    bool foundJunction = false;
    while((routeLength - offset) > 0)
    {
        bool found = false;
        
        /*
         * We have no other choice than checking all routes for all listed subnets. Hopefully:
         * -route size, as an integer value, remains always fairly low (it's rare to have route 
         *  lengths above 30 hops) and has an absolute upper bound (max TTL value = 255), 
         * -the distance between the neighborhood and the next junction is also often very low 
         *  (1 in ideal cases), so finding the next junction IP should be quick when it exists.
         * One can therefore consider this part to run in O(N) (N = amount of listed subnets).
         */
        
        for(list<SubnetSite*>::iterator it = subnets.begin(); it != subnets.end(); ++it)
        {
            RouteInterface *curRoute = (*it)->getRoute();
            InetAddress IPInterface = curRoute[routeLength - 1 - offset].ip;
            
            /*
             * In case of long cycles (e.g., given W, X, Y, Z, we have the route ..., W, X, Y, W, 
             * Z), it's possible a peer is actually the same IP as the label IP. This is very 
             * rare, but when it occurs, the neighborhood which the peer IP is the same as the IP 
             * of its label might not appear in the graph.
             */
            
            if(IPInterface == labelIP)
                continue;
            
            IPTableEntry *entry = dict->lookUp(IPInterface);
            if(entry != NULL)
            {
                if(entry->isANeighborhoodLabel())
                {
                    found = true;
                    break;
                }
            }
        }
        
        if(found)
        {
            foundJunction = true;
            break;
        }
    
        offset++;
    }
    
    // No junction IP, no peer (or rather, empty list)
    if(!foundJunction)
        return result;
    
    // 2) List all possible IP interfaces appearing at that distance
    map<InetAddress, bool> IPSet;
    for(list<SubnetSite*>::iterator i = subnets.begin(); i != subnets.end(); ++i)
    {
        RouteInterface *curRoute = (*i)->getRoute();
        InetAddress IPInterface = curRoute[routeLength - 1 - offset].ip;
        
        // Same reason as above.
        if(IPInterface == labelIP)
            continue;
        
        bool isJunction = false;
        IPTableEntry *entry = dict->lookUp(IPInterface);
        if(entry != NULL && entry->isANeighborhoodLabel())
            isJunction = true;
        
        map<InetAddress, bool>::iterator res = IPSet.find(IPInterface);
        if(res == IPSet.end())
            IPSet.insert(pair<InetAddress, bool>(IPInterface, isJunction));
    }
    
    // possibleIPs = all IPs from the set as a list, junctionIPs = only neighborhood labels
    list<InetAddress> possibleIPs;
    list<InetAddress> junctionIPs;
    for(map<InetAddress, bool>::iterator it = IPSet.begin(); it != IPSet.end(); ++it)
    {
        InetAddress IP = it->first;
        possibleIPs.push_back(IP);
        if(it->second)
            junctionIPs.push_back(IP);
    }
    possibleIPs.sort(InetAddress::smaller);
    junctionIPs.sort(InetAddress::smaller);
    
    // 3) Alias resolution if several possible IPs and at least one junction
    if(possibleIPs.size() > 1 && junctionIPs.size() > 0)
    {
        ostream *out = env->getOutputStream();
        
        (*out) << "Conducting alias resolution ";
        if(junctionIPs.size() > 1)
        {
            if(junctionIPs.size() < possibleIPs.size())
                (*out) << "on " << possibleIPs.size() << " IPs containing junction IPs ";
            else
                (*out) << "on " << junctionIPs.size() << " junction IPs ";
            for(list<InetAddress>::iterator i = junctionIPs.begin(); i != junctionIPs.end(); ++i)
            {
                if(i != junctionIPs.begin())
                    (*out) << ", ";
                (*out) << (*i);
            }
        }
        else
            (*out) << "on " << possibleIPs.size() << " IPs containing junction IP " << junctionIPs.front();
        (*out) << "... " << std::flush;
        
        if(this->ahc->isPrintingSteps())
        {
            (*out) << endl;
            if(this->ahc->debugMode()) // Additionnal line break for harmonious display
                (*out) << endl;
        }
        
        ahc->setIPsToProbe(possibleIPs);
        ahc->collect();
        
        list<Fingerprint> fingerprints;
        list<Router*> aliases = ar->resolve(possibleIPs, &fingerprints);
        (*out) << "IP entries:" << endl;
        for(list<Fingerprint>::iterator i = fingerprints.begin(); i != fingerprints.end(); ++i)
            (*out) << ((*i).ipEntry)->toString() << endl;
        
        // Aliases found
        if(aliases.size() > 0)
        {
            (*out) << "Discovered ";
            if(aliases.size() > 1)
            {
                (*out) << aliases.size() << " aliases:" << endl;
                for(list<Router*>::iterator it = aliases.begin(); it != aliases.end(); ++it)
                {
                    (*out) << (*it)->toString() << endl;
                }
            }
            else
            {
                (*out) << "one alias: " << aliases.front()->toString() << "." << endl;
            }
            
            list<InetAddress> excluded; // I.e. junction IPs not present in aliases
            for(list<InetAddress>::iterator i = junctionIPs.begin(); i != junctionIPs.end(); ++i)
            {
                bool found = false;
                for(list<Router*>::iterator j = aliases.begin(); j != aliases.end(); ++j)
                {
                    if((*j)->hasInterface((*i)))
                    {
                        found = true;
                        break;
                    }
                }
                
                if(!found)
                    excluded.push_back((*i));
            }
            
            // If excluded has the same size as junctionIPs, then aliases are irrelevant.
            if(excluded.size() == junctionIPs.size())
            {
                (*out) << "No alias with a junction IP discovered. Will consider any ";
                (*out) << "junction IP as a peer.\n";
                for(list<InetAddress>::iterator i = junctionIPs.begin(); i != junctionIPs.end(); ++i)
                {
                    Peer *Gynt = new Peer(offset, (*i));
                    result.push_back(Gynt);
                }
            }
            else
            {
                // Creates Peer objects with aliases
                for(list<Router*>::iterator i = aliases.begin(); i != aliases.end(); ++i)
                {
                    Peer *Gynt = new Peer(offset, (*i));
                    result.push_back(Gynt);
                }
                
                // If there are excluded junction IPs, creates additionnal Peer objects
                for(list<InetAddress>::iterator i = excluded.begin(); i != excluded.end(); ++i)
                {
                    Peer *Gynt = new Peer(offset, (*i));
                    result.push_back(Gynt);
                }
            }
            
            // Cleans discovered aliases
            for(list<Router*>::iterator i = aliases.begin(); i != aliases.end(); ++i)
                delete (*i);
            aliases.clear();
        }
        // No alias discovered
        else
        {
            (*out) << "No alias discovered. Will consider any junction IP as a peer.\n";
            for(list<InetAddress>::iterator i = junctionIPs.begin(); i != junctionIPs.end(); ++i)
            {
                Peer *Gynt = new Peer(offset, (*i));
                result.push_back(Gynt);
            }
        }
    }
    else if(possibleIPs.size() == 1 && junctionIPs.size() == 1) // Condition to ensure consistency
    {
        Peer *Gynt = new Peer(offset, possibleIPs.front());
        result.push_back(Gynt);
    }
    
    return result;
}

void GraphBuilder::postProcessPeers(Aggregate *a)
{
    // Due to aggregates rarely having a lot of peers (>10 is already exception), 0(N²) is OK here
    list<Peer*> *peers = a->getPeers();
    for(list<Peer*>::iterator i = peers->begin(); i != peers->end(); ++i)
    {
        Peer *prev = (*i);
        list<Peer*>::iterator j = i;
        j++;
        while(j != peers->end())
        {
            Peer *cur = (*j);
            if(prev->isSimilarTo(cur))
            {
                if(!prev->equals(cur))
                    prev->mergeWith(cur);
                peers->erase(j--);
                delete cur;
            }
            j++;
        }
    }
    peers->sort(Peer::compare);
}

void GraphBuilder::postProcessPeers(Cluster *c)
{
    // Simpler version of the previous method
    list<Peer*> *peers = c->getPeers();
    for(list<Peer*>::iterator i = peers->begin(); i != peers->end(); ++i)
    {
        Peer *prev = (*i);
        list<Peer*>::iterator j = i;
        j++;
        while(j != peers->end())
        {
            Peer *cur = (*j);
            if(prev->equals(cur))
            {
                peers->erase(j--);
                delete cur;
            }
            j++;
        }
    }
}

void GraphBuilder::registerPreAlias(Peer *peer)
{
    IPLookUpTable *dict = env->getIPTable();
    list<InetAddress> *IPs = peer->getInterfaces();
    for(list<InetAddress>::iterator i = IPs->begin(); i != IPs->end(); i++)
    {
        InetAddress ip1 = (*i);
        IPTableEntry *ipEntry1 = dict->lookUp(ip1);
        if(ipEntry1 == NULL)
            continue;
        
        for(list<InetAddress>::iterator j = i; j != IPs->end(); j++)
        {
            if(i == j)
                continue;
            
            InetAddress ip2 = (*j);
            IPTableEntry *ipEntry2 = dict->lookUp(ip2);
                
            if(ipEntry2 == NULL)
                continue;
                
            ipEntry1->recordPreAlias(ip2);
            ipEntry2->recordPreAlias(ip1);
        }
    }
}

void GraphBuilder::connectToPeers(Neighborhood *n)
{
    list<Peer*> *peers = n->getPeers();
    unsigned short connectedPeers = 0; // For defensive programming
    while(peers->size() > 0)
    {
        // Gets concrete peer (= neighborhood) and deletes the Peer object (no longer relevant)
        Peer *nextPeer = peers->front();
        peers->pop_front();
        
        unsigned short offset = nextPeer->getOffset();
        Neighborhood *next = nextPeer->getConcretePeer();
        
        if(next == NULL)
        {
            /*
             * Defensive programming: peer is deleted since there's no concrete peer to link to. 
             * If there is no more peer and no connection could be established with this 
             * neighborhood, add it as a gate of the graph.
             */
            
            anomalies << "Connection error: " << nextPeer->toString();
            anomalies << " had no concrete peer." << endl;
            
            delete nextPeer;
            if(peers->size() == 0 && connectedPeers == 0)
            {
                result->addGate(n);
                anomalies << "Since " << n->getFullLabel() << " had no other peer, it became ";
                anomalies << "a gate of the graph." << endl;
                return;   
            }
            else
            {
                continue;
            }
        }
        
        Edge *newLink = NULL;
        
        delete nextPeer;
        
        // Case of a direct connection
        if(offset == 1)
        {
            // Slightly more complex case: the neighborhood is a cluster
            if(Cluster* cluster = dynamic_cast<Cluster*>(n))
            {
                list<InetAddress> *labels = cluster->getLabels();
                
                // 1) Is there a connecting subnet ?
                SubnetSite *medium = NULL;
                for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
                {
                    medium = next->getConnectingSubnet((*i));
                    if(medium != NULL)
                        break;
                }
                
                if(medium != NULL)
                {
                    newLink = new DirectLink(next, n, medium);
                }
                // 2) If not, we have an indirect link (+ look-up of a possible medium)
                else
                {
                    SubnetMapEntry *mediumBis = NULL;
                    for(list<InetAddress>::iterator i = labels->begin(); i != labels->end(); ++i)
                    {
                        mediumBis = result->getSubnetContaining((*i));
                        if(mediumBis != NULL)
                            break;
                    }
                    
                    if(mediumBis != NULL)
                    {
                        newLink = new IndirectLink(next, n, mediumBis);
                    }
                    else
                    {
                        newLink = new IndirectLink(next, n);
                    }
                }
            }
            // Simple case: Node neighborhood
            else if(Node* node = dynamic_cast<Node*>(n))
            {
                InetAddress label = node->getLabel();
            
                // 1) Is there a connecting subnet ?
                SubnetSite *medium = next->getConnectingSubnet(label);
                if(medium != NULL)
                {
                    newLink = new DirectLink(next, n, medium);
                }
                // 2) If not, we have an indirect link (+ look-up of a possible medium)
                else
                {
                    SubnetMapEntry *mediumBis = result->getSubnetContaining(label);
                    if(mediumBis != NULL)
                    {
                        newLink = new IndirectLink(next, n, mediumBis);
                    }
                    else
                    {
                        newLink = new IndirectLink(next, n);
                    }
                }
            }
        }
        // Case of a remote connection between the neighborhoods: MiscHop objects needed
        else
        {
            // 1) Enumerate routes connecting both neighborhoods
            list<SubnetSite*> *subnets = n->getSubnets();
            list<SubnetSite*> routes; // Routes to connect the neighborhoods
            unsigned short nbAnomalies = subnets->front()->getNeighborhoodLabelAnomalies();
            unsigned short routeSize = subnets->front()->getFinalRouteSize();
            unsigned short index = routeSize - 1 - nbAnomalies - offset;
            for(list<SubnetSite*>::iterator i = subnets->begin(); i != subnets->end(); ++i)
            {
                RouteInterface *curRoute = (*i)->getRoute();
                if(Cluster* cluster = dynamic_cast<Cluster*>(next))
                {
                    list<InetAddress> *labels = cluster->getLabels();
                    for(list<InetAddress>::iterator j = labels->begin(); j != labels->end(); ++j)
                    {
                        if(curRoute[index].ip == (*j))
                        {
                            routes.push_back((*i));
                            break;
                        }
                    }
                }
                else if(Node* node = dynamic_cast<Node*>(next))
                {
                    if(curRoute[index].ip == node->getLabel())
                        routes.push_back((*i));
                }
            }
            
            // No route was found, so a fully anonymous path will be used instead
            if(routes.size() == 0)
            {
                anomalies << "Since there is no route to connect " << n->getFullLabel();
                anomalies << " with " << next->getFullLabel() << ", their link will be ";
                anomalies << "modeled by a fully anonymous path." << endl;
                newLink = new RemoteLink(next, n);
            }
            else
            {
                // 2) Using a map, isolates unique routes (need at least one non-anonymous hop)
                map<string, list<InetAddress> > uniqueRoutes;
                for(list<SubnetSite*>::iterator i = routes.begin(); i != routes.end(); ++i)
                {
                    RouteInterface *curRoute = (*i)->getRoute();
                    list<InetAddress> route;
                    stringstream routeSs;
                    bool fullyAnonymous = true;
                    for(unsigned short j = index + 1; j < routeSize - 1 - nbAnomalies; j++)
                    {
                        if(curRoute[j].ip != InetAddress(0))
                            fullyAnonymous = false;
                    
                        if(j > index + 1)
                            routeSs << ", ";
                        routeSs << curRoute[j].ip;
                        route.push_back(curRoute[j].ip);
                    }
                    
                    if(fullyAnonymous)
                        continue;
                    
                    string routeStr = routeSs.str();
                    map<string, list<InetAddress> >::iterator res = uniqueRoutes.find(routeStr);
                    if(res == uniqueRoutes.end())
                        uniqueRoutes.insert(pair<string, list<InetAddress> >(routeStr, route));
                }
                
                if(uniqueRoutes.size() > 0)
                {
                    // 4) For each unique route, creates the miscellaneous hops or connects existing one
                    map<string, list<InetAddress> >::iterator i;
                    list<MiscHop*> entryPoints;
                    for(i = uniqueRoutes.begin(); i != uniqueRoutes.end(); ++i)
                    {
                        list<InetAddress> route = i->second;
                        unsigned char TTL = (unsigned char) (index + 2);
                        
                        // Entry point of the path
                        MiscHop *first;
                        InetAddress firstIP = route.front();
                        route.pop_front();
                        if(firstIP == InetAddress(0))
                            first = result->getMiscHop(TTL);
                        else
                            first = result->getMiscHop(firstIP);
                        entryPoints.push_back(first);
                        
                        // Rest of the path
                        MiscHop *prev = first, *cur;
                        while(route.size() > 0)
                        {
                            TTL++;
                            InetAddress nextIP = route.front();
                            route.pop_front();
                            if(nextIP != InetAddress(0) && nextIP == prev->getIP()) // Cycle
                                continue;
                            
                            if(nextIP == InetAddress(0))
                                cur = result->getMiscHop(TTL);
                            else
                                cur = result->getMiscHop(nextIP);
                            
                            prev->connectTo(cur);
                            prev = cur;
                        }
                        
                        // Connects last MiscHop object to the neighborhood
                        prev->connectTo(n);
                    }
                    
                    newLink = new RemoteLink(next, n, entryPoints);
                }
                else
                {
                    newLink = new RemoteLink(next, n);
                }
            }
        }
        
        next->addOutEdge(newLink);
        n->addInEdge(newLink);
        
        connectedPeers++;
        
        // Recursive call
        this->connectToPeers(next);
    }
}
