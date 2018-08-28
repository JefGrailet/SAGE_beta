/*
 * NetworkScanner.cpp
 *
 *  Created on: Nov 14, 2016
 *      Author: jefgrailet
 *
 * Implements the class defined in NetworkScanner.h (see this file to learn further about the 
 * goals of such a class).
 */

#include "NetworkScanner.h"
#include "./explorenet/ExploreNETRunnable.h"
#include "../../common/thread/Thread.h"

NetworkScanner::NetworkScanner(Environment *env)
{
    this->env = env;
    this->sr = new SubnetRefiner(this->env);
}

NetworkScanner::~NetworkScanner()
{
    delete sr;
}

void NetworkScanner::scan(list<InetAddress> targets)
{
    // Retrieves output stream, display mode and subnet sets
    ostream *out = env->getOutputStream();
    unsigned short displayMode = env->getDisplayMode();
    SubnetSiteSet *subnetSet = env->getSubnetSet();
    SubnetSiteSet *zonesToAvoid = env->getIPBlocksToAvoid();
    
    /*
     * TIMEOUT ADAPTATION
     *
     * Adapts the timeout value if the targets are close (as unsigned long int). When the 
     * gap between addresses is small, it is preferrable to increase the timeout period 
     * during the subnet inference/refinement in case it generated too much traffic at a 
     * particular network location.
     */
    
    TimeVal timeoutPeriod = env->getTimeoutPeriod();
    unsigned long smallestGap = 0;
    InetAddress previous(0);
    for(list<InetAddress>::iterator it = targets.begin(); it != targets.end(); ++it)
    {
        InetAddress cur = (*it);
        
        if(previous.getULongAddress() == 0)
        {
            previous = cur;
            continue;
        }
        
        unsigned long curGap = 0;
        if(cur.getULongAddress() > previous.getULongAddress())
            curGap = cur.getULongAddress() - previous.getULongAddress();
        else
            curGap = previous.getULongAddress() - cur.getULongAddress();
        
        if(smallestGap == 0 || curGap < smallestGap)
            smallestGap = curGap;
        
        previous = cur;
    }
    
    bool editedTimeout = false;
    if(smallestGap < 64)
    {
        env->setTimeoutPeriod(timeoutPeriod * 2);
        editedTimeout = true;
        (*out) << "Timeout adapted for network scanning: " << env->getTimeoutPeriod();
        (*out) << "\n" << endl;
    }

    // Size of threads vector
    unsigned short nbThreads = env->getMaxThreads();
    unsigned short sizeArray;
    if(targets.size() > (unsigned int) nbThreads)
        sizeArray = nbThreads;
    else
        sizeArray = (unsigned short) targets.size();
    
    // Creates thread(s)
    Thread **th = new Thread*[sizeArray];
    for(unsigned short i = 0; i < sizeArray; i++)
        th[i] = NULL;

    (*out) << "Starting network scanning..." << endl;
    if(!(displayMode == Environment::DISPLAY_MODE_DEBUG || displayMode == Environment::DISPLAY_MODE_VERBOSE))
        (*out) << endl;
    
    while(targets.size() > 0)
    {
        unsigned short curNbThreads;
        if(targets.size() > (unsigned int) nbThreads)
            curNbThreads = nbThreads;
        else
            curNbThreads = (unsigned short) targets.size();
        
        unsigned short range = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID;
        range -= DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID;
        range /= curNbThreads;
        
        for(unsigned short i = 0; i < curNbThreads; i++)
        {
            InetAddress curTarget(targets.front());
            targets.pop_front();
            
            unsigned short lowerBound = (i * range);
            unsigned short upperBound = lowerBound + range - 1;
            
            Runnable *task = NULL;
            try
            {
                task = new ExploreNETRunnable(env, 
                                              curTarget, 
                                              DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + lowerBound, 
                                              DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID + upperBound, 
                                              DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ, 
                                              DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);
                
                th[i] = new Thread(task);
            }
            catch(SocketException &se)
            {
                for(unsigned short j = 0; j < curNbThreads; j++)
                {
                    delete th[j];
                    th[j] = NULL;
                }
                
                delete[] th;
                throw StopException();
            }
            catch(ThreadException &te)
            {
                delete task;
            
                for(unsigned short j = 0; j < curNbThreads; j++)
                {
                    delete th[j];
                    th[j] = NULL;
                }
                
                delete[] th;
                throw StopException();
            }
        }

        // Launches thread(s) then waits for completion
        for(unsigned short i = 0; i < curNbThreads; i++)
        {
            if(th[i] != NULL)
            {
                th[i]->start(); // N.B.: each thread will write in the console
                Thread::invokeSleep(env->getProbeThreadDelay());
            }
        }
        
        for(unsigned short i = 0; i < curNbThreads; i++)
        {
            if(th[i] != NULL)
            {
                th[i]->join();
                delete th[i];
                th[i] = NULL;
            }
        }
        
        (*out) << endl;
        
        if(env->isStopping())
        {
            delete[] th;
            throw StopException();
        }
        
        /*
         * BYPASS
         *
         * After probing a certain amount of target addresses, the scanning temporarily stops in 
         * order to perform a first form of refinement: expansion. Indeed, ExploreNET tends to 
         * partition subnets when they do not feature a certain amount of responsive interfaces 
         * spread in a quasi-uniform fashion in the subnet. Expansion aims at correcting this 
         * issue by relying on the notion of contra-pivot (see SubnetRefiner.cpp/.h for more 
         * details about this).
         *
         * Because expansion can eventually lead to the discovery of large subnets, performing 
         * this refinement may avoid probing several addresses that are already inside the 
         * boundaries of the refined subnets. Therefore, this should speed up the scanning which 
         * is otherwise a time-consuming operation (see the target filtering step).
         */
        
        // List of subnets to handle: newly discovered subnets and incomplete subnets (toRefine)
        list<SubnetSite*> discovered;
        list<SubnetSite*> toRefine = subnetSet->postProcessNewSubnets(&discovered);
        
        // Displays the discovered subnets
        size_t nbDiscovered = discovered.size();
        if(nbDiscovered > 0)
        {
            (*out) << "New subnets found by the previous " << curNbThreads << " threads:" << endl;
            for(list<SubnetSite*>::iterator it = discovered.begin(); it != discovered.end(); ++it)
            {
                SubnetSite *ss = (*it);
                unsigned short status = ss->getStatus();
                (*out) << ss->getInferredNetworkAddressString() << ": ";
                if(status == SubnetSite::INCOMPLETE_SUBNET)
                    (*out) << "incomplete";
                else if(status == SubnetSite::ACCURATE_SUBNET)
                    (*out) << "accurate";
                else
                    (*out) << "odd";
                (*out) << " subnet" << endl;
            }
            (*out) << endl;
        }
        else
        {
            (*out) << "Previous " << curNbThreads << " threads found no new subnet.\n" << endl;
        }
        
        // Performs refinement if needed
        if(toRefine.size() > 0)
        {
            (*out) << "Refining incomplete subnets...\n" << endl;
            
            /*
             * November 2017: new heuristic. When subnets have the same first 20 bits for their 
             * respective prefix, they are sorted in decreasing order. The idea is that if the 
             * "upper" subnets in the /20 spectrum are expanded, they will cover "lower" subnets 
             * which won't have to be expanded. The contrary operation would result in re-probing 
             * some targets due to the full subnet only being discovered after expanding the chunk 
             * that appears the farther in the spectrum.
             */
            
            toRefine.sort(SubnetSite::compareAlt);
            while(toRefine.size() > 0)
            {
                // Gets an incomplete subnet and removes it from the list
                SubnetSite *curCandidate = toRefine.front();
                toRefine.pop_front();
            
                /*
                 * Checks if expansion should be conducted, i.e. the subnet should not be 
                 * encompassed by an undefined subnet found in the "IPBlocksToAvoid" set from 
                 * Environment. Otherwise, expansion is a waste of time: the presence of an 
                 * undefined subnet means we already looked for Contra-Pivot interfaces in this 
                 * zone without success. The subnet is directly labelled as shadow and reinserted 
                 * in subnetSet.
                 */
                
                SubnetSite *toAvoid = zonesToAvoid->getEncompassingSubnet(curCandidate);
                if(toAvoid != NULL)
                {
                    string curStr = curCandidate->getInferredNetworkAddressString();
                    string toAvoidStr = toAvoid->getInferredNetworkAddressString();
                    (*out) << "No refinement for " << curStr << ": it is encompassed in the ";
                    (*out) << toAvoidStr << " IPv4 address block.\nThis block features Pivot ";
                    (*out) << "interfaces with the same TTL as expected for " << curStr << ".\n";
                    (*out) << "It has already been checked to find Contra-Pivot interfaces, ";
                    (*out) << "without success.\n";
                    (*out) << curStr << " marked as SHADOW subnet.\n" << endl;
                
                    curCandidate->setStatus(SubnetSite::SHADOW_SUBNET);
                    
                    // No check of return value because subnet did not change
                    subnetSet->addSite(curCandidate);
                    continue;
                }
                
                try
                {
                    sr->expand(curCandidate);
                }
                catch(StopException &se)
                {
                    // Deletes remaining resources involved in scanning
                    delete curCandidate;
                    for(list<SubnetSite*>::iterator i = toRefine.begin(); i != toRefine.end(); ++i)
                        delete (*i);
                    delete[] th;
                    
                    // Re-throws
                    throw;
                }
                
                // Removes incomplete subnets that are now encompassed by curCandidate (+ merging)
                for(list<SubnetSite*>::iterator i = toRefine.begin(); i != toRefine.end(); ++i)
                {
                    SubnetSite *ss = (*i);
                    if(curCandidate->encompasses(ss))
                    {
                        curCandidate->mergeNodesWith(ss);
                        delete ss;
                        toRefine.erase(i--);
                    }
                }
                
                // Adds the refined subnet (deletes it if smaller or equivalent to a known subnet)
                unsigned short res = subnetSet->addSite(curCandidate);
                if(res == SubnetSiteSet::SMALLER_SUBNET || res == SubnetSiteSet::KNOWN_SUBNET)
                    delete curCandidate;
            }
            
            /*
             * If we are in laconic display mode, we add a line break before the next message 
             * to keep the display pretty to read.
             */
            
            if(displayMode == Environment::DISPLAY_MODE_LACONIC)
                (*out) << "\n";
            
            (*out) << "Finished refining newly discovered incomplete subnets.\n" << endl;
        }
        
        /*
         * TARGET FILTERING
         *
         * Using a list of all the new subnets that appeared in this round (and after 
         * refinement, if it occurred) obtained via the SubnetSiteSet class, cleans the list of 
         * targets from IPs that are encompassed by the new subnets and print a small message to 
         * advertise how many IPs were avoided.
         */
        
        if(nbDiscovered > 0)
        {
            list<SubnetSite*> finalLs = subnetSet->listNewAndRefinedSubnets();
            if(finalLs.size() > 0)
            {
                IPLookUpTable *dict = env->getIPTable();
                
                unsigned int skippedIPs = 0;
                unsigned int expectedIPs = 0;
                for(list<InetAddress>::iterator i = targets.begin(); i != targets.end(); ++i)
                {
                    InetAddress targetIP = (*i);
                    for(list<SubnetSite*>::iterator j = finalLs.begin(); j != finalLs.end(); ++j)
                    {
                        SubnetSite *ss = (*j);
                        IPTableEntry *IPEntry = dict->lookUp(targetIP);
                        if(IPEntry == NULL)
                            IPEntry = dict->create(targetIP);
                        
                        if(ss->contains(targetIP))
                        {
                            /*
                             * For the sake of accuracy, only the IPs that are already listed in 
                             * the subnet are filtered out. Indeed, non-listed IPs might include:
                             * -IPs that are no longer responsive (will be added thanks to 
                             *  subsequent probing or refinement by filling), 
                             * -outliers (IPs with larger TTL than pivot, despite being on the 
                             *  subnet adress space).
                             * An "expected" TTL (which is the pivot TTL of the encompassing 
                             * subnet) is recorded in the IP dictionnary for the non-listed IPs, 
                             * such that obtaining a reply from these IPs later does not trigger 
                             * the whole subnet inference process and stops at distance 
                             * evaluation, in order to check whether the IP was an outlier or just 
                             * a (contra-)pivot IP that wasn't responsive earlier. N.B.: expected 
                             * TTL was added in April 2018.
                             */
                            
                            SubnetSiteNode *ssn = ss->getNode(targetIP);
                            if(ssn != NULL)
                            {
                                IPEntry->setTTL(ssn->TTL);
                                targets.erase(i--);
                                skippedIPs++;
                            }
                            else
                            {
                                unsigned char pivotTTL = ss->getShortestTTL();
                                if(pivotTTL < ss->getGreatestTTL())
                                    pivotTTL += 1;
                                IPEntry->setExpectedTTL(pivotTTL);
                                expectedIPs++;
                            }
                        }
                    }
                }
                
                if(skippedIPs > 0)
                {
                    if(skippedIPs > 1)
                    {
                        (*out) << skippedIPs << " IPs belonging to newly discovered subnets were ";
                        (*out) << "removed from the list of targets." << endl;
                    }
                    else
                    {
                        (*out) << "One IP belonging to a newly discovered subnet was removed from ";
                        (*out) << "the list of targets." << endl;
                    }
                }
                if(expectedIPs > 0)
                {
                    if(expectedIPs > 1)
                    {
                        (*out) << "An expected TTL value has been set for " << expectedIPs;
                        (*out) << " IPs belonging to newly discovered subnets." << endl;
                    }
                    else
                    {
                        (*out) << "An expected TTL value has been set for one IP belonging to a ";
                        (*out) << "newly discovered subnet." << endl;
                    }
                }
                
                // For harmonious display
                if(skippedIPs > 0 || expectedIPs > 0)
                {
                    (*out) << endl;
                }
            }
        }
        
        if(env->isStopping())
        {
            break;
        }
    }
    
    delete[] th;
    
    (*out) << "Scanning completed.\n" << endl;
    
    // Restores regular timeout
    if(editedTimeout)
    {
        env->setTimeoutPeriod(timeoutPeriod);
    }
}

void NetworkScanner::finalize()
{
    /*
     * END OF SUBNET REFINEMENT
     *
     * After the scanning, subnets may still not contain all live interfaces in their list: a 
     * filling method helps to fix this issue by adding unlisted responsive interfaces that are 
     * within the boundaries of the subnet.
     *
     * Shadow subnets, if any, are also expanded so that their size (determined by their prefix) 
     * is the maximum size for these subnets to not collide with other inferred subnets that are 
     * incompatible for merging. In other words, it computes a lower bound on the prefix length, 
     * therefore an upper bound on the size of the subnet.
     *
     * Note that this step, unlike scanning, is entirely passive.
     */

    ostream *out = env->getOutputStream();
    unsigned short displayMode = env->getDisplayMode();
    SubnetSiteSet *subnetSet = env->getSubnetSet();

    int nbShadows = 0;
    list<SubnetSite*> *ssList = subnetSet->getSubnetSiteList();
    list<SubnetSite*> toFill;
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        // Just in case
        if((*it) == NULL)
            continue;
    
        unsigned short status = (*it)->getStatus();
        if(status == SubnetSite::ACCURATE_SUBNET || status == SubnetSite::ODD_SUBNET)
            toFill.push_back((*it));
        else if((*it)->getStatus() == SubnetSite::SHADOW_SUBNET)
            nbShadows++;
    }
    
    // Filling
    if(toFill.size() > 0)
    {
        (*out) << "Refinement by filling (passive method)...\n" << endl;
        
        for(list<SubnetSite*>::iterator it = toFill.begin(); it != toFill.end(); ++it)
        {
            // Just in case
            if((*it) == NULL)
                continue;
            
            unsigned short status = (*it)->getStatus();
            
            sr->fill(*it);
            if((*it) != NULL && status == SubnetSite::ACCURATE_SUBNET)
                (*it)->recomputeRefinementStatus();
        }
        
        /*
         * Like at the end of Bypass round, we have to take into account the display mode 
         * before adding a new line break to space harmoniously the different steps.
         */
        
        if(displayMode != Environment::DISPLAY_MODE_LACONIC)
            (*out) << endl;
    }
    
    // Shadow expansion (no parallelization here, because this operation is instantaneous)
    if(nbShadows > 0)
    {
        (*out) << "Expanding shadow subnets to the maximum...\n" << endl;
        for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
        {
            if((*it)->getStatus() == SubnetSite::SHADOW_SUBNET)
            {
                sr->shadowExpand(*it);
            }
        }
        
        /*
         * Removes all shadow subnets, then puts them back. The motivation is to merge the subnets 
         * that have the same prefix length after shadow expansion, because it is rather frequent 
         * that several incomplete subnets lead to the same shadow subnet.
         *
         * It can also occur that a shadow subnet actually contains one or several outliers 
         * from another subnet. In that case, it should be merged with the larger subnet.
         */
        
        SubnetSite *shadow = subnetSet->getShadowSubnet();
        list<SubnetSite*> listShadows;
        while(shadow != NULL)
        {
            listShadows.push_back(shadow);
            shadow = subnetSet->getShadowSubnet();
        }
        
        list<SubnetSite*>::iterator listBegin = listShadows.begin();
        list<SubnetSite*>::iterator listEnd = listShadows.end();
        for(list<SubnetSite*>::iterator it = listBegin; it != listEnd; ++it)
        {
            unsigned short res = subnetSet->addSite((*it));
            if(res == SubnetSiteSet::SMALLER_SUBNET || res == SubnetSiteSet::KNOWN_SUBNET)
            {
                delete (*it);
            }
        }
    }
}
