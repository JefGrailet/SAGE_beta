/*
 * AliasResolver.cpp
 *
 *  Created on: Oct 20, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in AliasResolver.h (see this file to learn further about the goals 
 * of such a class).
 */

#include <list>
using std::list;
#include <cmath>

#include "AliasResolver.h"

AliasResolver::AliasResolver(Environment *env)
{
    this->env = env;
}

AliasResolver::~AliasResolver()
{
}

unsigned short AliasResolver::Ally(IPTableEntry *ip1, IPTableEntry *ip2, unsigned short maxDiff)
{
    // The first condition is just in case; normally calling code checks it first
    if(ip1 != NULL && ip1->hasIPIDData() && ip2 != NULL && ip2->hasIPIDData())
    {
        unsigned short nbIPIDs = env->getARNbIPIDs();
        
        /*
         * February 2018: the Ally() method has now a new implementation. Thanks to the new 
         * IP-ID collection mechanism introduced back in 2017, looking for an interleaving 
         * sequence of IP-IDs between both IPs is now much simpler. Indeed, thanks to the "round 
         * probing", no second IP-ID in a sequence for an IP should come earlier than any first 
         * IP-ID, and this also holds through for second and third IP-ID, etc. Therefore, one can 
         * just compare IP-ID by IP-ID. This also allows for a more thorough resolution, as it is 
         * also easier to check that the whole sequence of IP-IDs for both IPs match.
         */
        
        unsigned short curIndex = 0;
        unsigned short nbRollovers = 0;
        
        // 1) Checks the token;IP-ID pairs are sound w.r.t. each other
        while(curIndex < nbIPIDs)
        {
            unsigned int ip1Token = ip1->getProbeToken(curIndex);
            unsigned int ip2Token = ip2->getProbeToken(curIndex);
            
            unsigned short ip1ID = ip1->getIPIdentifier(curIndex);
            unsigned short ip2ID = ip2->getIPIdentifier(curIndex);
            
            // IP-ID from first IP comes first
            if(ip1Token < ip2Token)
            {
                // But IP-ID from first IP is bigger: is it a rollover scenario ?
                if(ip1ID > ip2ID)
                {
                    unsigned int diff = (65535 - ip1ID) + ip2ID;
                
                    /*
                     * If the difference is too large, then it cannot be a rollover scenario. The 
                     * alias can't be validated.
                     */
                    
                    if(diff > maxDiff) 
                        return ALLY_REJECTED;
                    nbRollovers++;
                }
            }
            // IP-ID from second IP comes first
            else
            {
                if(ip2ID > ip1ID)
                {
                    unsigned int diff = (65535 - ip2ID) + ip1ID;
                    if(diff > maxDiff) 
                        return ALLY_REJECTED;
                    nbRollovers++;
                }
            }
            
            // More than one rollover suggests this alias cannot be aliased soundly with Ally.
            if(nbRollovers > 1)
                return ALLY_REJECTED;
            
            curIndex++;
        }
        
        // 2) Checks the sequence is sound
        curIndex = 0;
        nbRollovers = 0;
        while(curIndex < nbIPIDs - 1)
        {
            unsigned short ip1ID1 = ip1->getIPIdentifier(curIndex);
            unsigned short ip2ID1 = ip2->getIPIdentifier(curIndex);
            
            unsigned short ip1ID2 = ip1->getIPIdentifier(curIndex + 1);
            unsigned short ip2ID2 = ip2->getIPIdentifier(curIndex + 1);
            
            // Possible rollover ? If yes, code checks if the difference (65535 - ID1) + ID2.
            if(ip1ID1 > ip2ID2)
            {
                unsigned int diff = (65535 - ip1ID1) + ip2ID2;
                
                /*
                 * If the difference is too large, then it cannot be a rollover scenario. The 
                 * alias can't be validated.
                 */
                
                if(diff > maxDiff) 
                    return ALLY_REJECTED;
                nbRollovers++;
            }
            
            // Same logic applies to the other couple (ip2ID1, ip1ID2)
            if(ip2ID1 > ip1ID2)
            {
                unsigned int diff = (65535 - ip2ID1) + ip1ID2;
                if(diff > maxDiff)
                    return ALLY_REJECTED;
                nbRollovers++;
            }
            
            // Too many rollovers suggest this alias cannot be aliased soundly with Ally.
            if(nbRollovers > 2)
                return ALLY_REJECTED;
            
            curIndex++;
        }
        
        return ALLY_ACCEPTED;
    }
    return ALLY_NO_SEQUENCE;
}

unsigned short AliasResolver::groupAlly(Fingerprint isolatedIP, 
                                        list<Fingerprint> group, 
                                        unsigned short maxDiff)
{
    IPTableEntry *candidateIP = isolatedIP.ipEntry;
    bool accepted = false;
    for(list<Fingerprint>::iterator it = group.begin(); it != group.end(); ++it)
    {
        IPTableEntry *curIP = (*it).ipEntry;
        
        unsigned short pairAllyRes = this->Ally(curIP, candidateIP, maxDiff);
        if(pairAllyRes == ALLY_ACCEPTED)
        {
            accepted = true;
        }
        else if(pairAllyRes == ALLY_REJECTED)
        {
            return ALLY_REJECTED;
        }
    }
    
    if(accepted)
        return ALLY_ACCEPTED;
    
    return ALLY_NO_SEQUENCE;
}

void AliasResolver::evaluateIPIDCounter(IPTableEntry *ip)
{
    // The first condition is just in case; normally calling code checks it first
    if(ip != NULL && ip->hasIPIDData())
    {
        // Alias resolution parameters
        unsigned short nbIPIDs = env->getARNbIPIDs();
        unsigned short maxRollovers = env->getARVelocityMaxRollovers();
        double maxError = env->getARVelocityMaxError();
        
        // Checks if this is an IP that just echoes the ID provided in probes
        unsigned short nbEchoes = 0;
        for(unsigned short i = 0; i < nbIPIDs; i++)
        {
            if(ip->getEcho(i))
            {
                nbEchoes++;
            }
        }
        
        if(nbEchoes == nbIPIDs)
        {
            ip->setCounterType(IPTableEntry::ECHO_COUNTER);
            ip->raiseFlagProcessed();
            return;
        }
        
        // Looks at the amount of negative deltas
        unsigned short negativeDeltas = 0;
        for(unsigned short i = 0; i < nbIPIDs - 1; i++)
        {
            unsigned short cur = ip->getIPIdentifier(i);
            unsigned short next = ip->getIPIdentifier(i + 1);
            if(cur > next)
            {
                negativeDeltas++;
            }
        }
        
        // If one or zero negative deltas, straightforward evaluation
        if(negativeDeltas < 2)
        {
            double v[nbIPIDs - 1];
            
            // Velocity computation
            for(unsigned short i = 0; i < nbIPIDs - 1; i++)
            {
                double b_i = (double) ip->getIPIdentifier(i);
                double b_i_plus_1 = (double) ip->getIPIdentifier(i + 1);
                double d_i = ip->getDelay(i);
                
                if(b_i_plus_1 > b_i)
                    v[i] = (b_i_plus_1 - b_i) / d_i;
                else
                    v[i] = (b_i_plus_1 + (65535 - b_i)) / d_i;
            }
            
            // Finds minimum/maximum velocity
            double maxV = v[0], minV = v[0];
            for(unsigned short i = 0; i < nbIPIDs - 1; i++)
            {
                if(v[i] > maxV)
                    maxV = v[i];
                if(v[i] < minV)
                    minV = v[i];
            }
            
            ip->setVelocityLowerBound(minV);
            ip->setVelocityUpperBound(maxV);
            ip->setCounterType(IPTableEntry::HEALTHY_COUNTER);
            ip->raiseFlagProcessed();
            return;
        }
    
        // Otherwise, solving equations becomes necessary.
        double d0 = (double) ip->getDelay(0);
        double b0 = (double) ip->getIPIdentifier(0);
        double b1 = (double) ip->getIPIdentifier(1);
        
        double x = 0.0;
        double v[nbIPIDs - 1];
        
        bool success = false;
        for(unsigned short i = 0; i < maxRollovers; i++)
        {
            // Computing speed for x
            if(b1 > b0)
                v[0] = (b1 - b0 + 65535 * x) / d0;
            else
                v[0] = (b1 + (65535 - b0) + 65535 * x) / d0;
            
            success = true;
            for(unsigned short j = 1; j < nbIPIDs - 1; j++)
            {
                // Computing speed for cur
                double b_j = (double) ip->getIPIdentifier(j);
                double b_j_plus_1 = (double) ip->getIPIdentifier(j + 1);
                double d_j = (double) ip->getDelay(j);
                double cur = 0.0;
                
                cur += (d_j / d0) * x;
                
                if(b_j_plus_1 > b_j)
                    cur -= ((b_j_plus_1 - b_j) / 65535);
                else
                    cur -= ((b_j_plus_1 + (65535 - b_j)) / 65535);
                
                if(b1 > b0)
                    cur += (((d_j * b1) - (d_j * b0)) / (65535 * d0));
                else
                    cur += (((d_j * b1) + (d_j * (65535 - b0))) / (65535 * d0));
                
                // Flooring/ceiling cur
                double floorCur = floor(cur);
                double ceilCur = ceil(cur);
                
                double selectedCur = 0.0;
                double gap = 0.0;
                
                if((cur - floorCur) > (ceilCur - cur))
                {
                    selectedCur = ceilCur;
                    gap = ceilCur - cur;
                }
                else
                {
                    selectedCur = floorCur;
                    gap = cur - floorCur;
                }
                
                // Storing speed of current time interval
                if(selectedCur > 0.0 && gap <= maxError)
                {
                    if(b_j_plus_1 > b_j)
                        v[j] = (b_j_plus_1 - b_j + 65535 * selectedCur) / d_j;
                    else
                        v[j] = (b_j_plus_1 + (65535 - b_j) + 65535 * selectedCur) / d_j;
                }
                else
                {
                    success = false;
                    break;
                }
            }
            
            if(success)
            {
                break;
            }
            
            x += 1.0;
        }
        
        if(success)
        {
            double maxV = v[0], minV = v[0];
            for(unsigned short i = 0; i < nbIPIDs - 1; i++)
            {
                if(v[i] > maxV)
                    maxV = v[i];
                if(v[i] < minV)
                    minV = v[i];
            }
            
            ip->setVelocityLowerBound(minV);
            ip->setVelocityUpperBound(maxV);
            ip->setCounterType(IPTableEntry::FAST_COUNTER);
        }
        else
        {
            // "Infinite" velocity: [0.0, 65535.0]; interpreted as a random counter
            ip->setVelocityLowerBound(0.0);
            ip->setVelocityUpperBound(65535.0);
            ip->setCounterType(IPTableEntry::RANDOM_COUNTER);
        }
        
        ip->raiseFlagProcessed();
    }
}

bool AliasResolver::velocityOverlap(IPTableEntry *ip1, IPTableEntry *ip2)
{
    if(ip1 == NULL || ip2 == NULL)
        return false;

    double low1 = ip1->getVelocityLowerBound();
    double low2 = ip2->getVelocityLowerBound();
    double up1 = ip1->getVelocityUpperBound();
    double up2 = ip2->getVelocityUpperBound();

    if(up1 == 0.0 || up2 == 0.0)
        return false;

    // Particular case of "infinite" velocity (should no longer be evaluated, but just in case)
    if(up1 == 65535.0 || up2 == 65535.0)
    {
        if(up1 == up2)
            return true;
        return false;
    }
    
    // Computes tolerance value
    double tolerance = env->getARVelocityBaseTolerance();
    if((low1 / (tolerance * 10)) > 1.0 && (low2 / (tolerance * 10)) > 1.0)
        tolerance *= (low1 / (tolerance * 10));
    
    // Finds largest intervall and extends it with tolerance
    double low1Bis = low1, low2Bis = low2, up1Bis = up1, up2Bis = up2;
    if((up1 - low1) > (up2 - low2))
    {
        low1Bis = low1 - tolerance;
        up1Bis = up1 + tolerance;
    }
    else
    {
        low2Bis = low2 - tolerance;
        up2Bis = up2 + tolerance;
    }
    
    // If overlaps with tolerance is OK...
    if(low1Bis <= up2Bis && up1Bis >= low2Bis)
    {
        unsigned short nbIPIDs = env->getARNbIPIDs();
        
        /*
         * (Short validation added in March 2017)
         *
         * If we denote ip_1 as the first probed IP, and ip_2 the second, we validate the 
         * association by checking if ip_2 first IP-ID belongs to the interval:
         *
         * [ip_1 last IP-ID, ip_1 last IP-ID + speed (inc./token) * 1,25 * (ip_2 first token - ip_1 last token)]
         *
         * The 1,25 ensures slight changes in speed do not disturb the final results. Overlaps 
         * must also be considered in the process.
         */
        
        IPTableEntry *ip_1, *ip_2;
        unsigned long diffTokens = 0;
        if(ip1->getProbeToken(nbIPIDs - 1) < ip2->getProbeToken(0))
        {
            ip_1 = ip1;
            ip_2 = ip2;
            diffTokens = ip2->getProbeToken(0) - ip1->getProbeToken(nbIPIDs - 1);
        }
        else
        {
            ip_1 = ip2;
            ip_2 = ip1;
            diffTokens = ip1->getProbeToken(0) - ip2->getProbeToken(nbIPIDs - 1);
        }
        
        // We compute a new speed, as increase per tokens, from the first IP.
        double speedPerToken = 0.0;
        unsigned int totalDiff = 0;
        for(unsigned short i = 0; i < nbIPIDs - 1; i++)
        {
            unsigned int ID1 = (unsigned int) ip_1->getIPIdentifier(i);
            unsigned int ID2 = (unsigned int) ip_1->getIPIdentifier(i + 1);
            
            if(ID2 < ID1)
                totalDiff += (ID2 + 65535 - ID1);
            else
                totalDiff += (ID2 - ID1);
        }
        speedPerToken = (double) totalDiff / (double) (ip_1->getProbeToken(nbIPIDs - 1) - ip_1->getProbeToken(0));
        speedPerToken *= 2;
        
        bool validated = false;
        unsigned short fIPID = ip_1->getIPIdentifier(nbIPIDs - 1);
        unsigned short lIPID = ip_2->getIPIdentifier(0);
        unsigned int prediction = (unsigned int) fIPID + (unsigned int) (speedPerToken * (double) diffTokens);
        if(prediction > 65535)
        {
            unsigned short endIPID = (unsigned short) (prediction % 65535);
            if((lIPID > fIPID) || (lIPID > 0 && lIPID <= endIPID))
                validated = true;
        }
        else
        {
            unsigned short endIPID = (unsigned short) prediction;
            if(lIPID > fIPID && lIPID <= endIPID)
                validated = true;
        }
        
        return validated;
    }
    return false;
}

bool AliasResolver::groupVelocity(Fingerprint isolatedIP, list<Fingerprint> group)
{
    IPTableEntry *candidateIP = isolatedIP.ipEntry;
    for(list<Fingerprint>::iterator it = group.begin(); it != group.end(); ++it)
    {
        IPTableEntry *curIP = (*it).ipEntry;
        
        if(!this->velocityOverlap(candidateIP, curIP))
        {
            return false;
        }
    }
    return true;
}

bool AliasResolver::reverseDNS(IPTableEntry *ip1, IPTableEntry *ip2)
{
    // Just in case
    if(ip1 == NULL || ip2 == NULL)
        return false;
    
    string hostName1 = ip1->getHostName();
    string hostName2 = ip2->getHostName();
    
    if(!hostName1.empty() && !hostName2.empty())
    {
        list<string> hn1Chunks;
        list<string> hn2Chunks;
        string element;
        
        stringstream hn1(hostName1);
        while (std::getline(hn1, element, '.'))
        {
            hn1Chunks.push_front(element);
        }
        
        stringstream hn2(hostName2);
        while (std::getline(hn2, element, '.'))
        {
            hn2Chunks.push_front(element);
        }
        
        if(hn1Chunks.size() == hn2Chunks.size())
        {
            unsigned short size = (unsigned short) hn1Chunks.size();
            unsigned short similarities = 0;
            
            list<string>::iterator itBis = hn1Chunks.begin();
            for(list<string>::iterator it = hn2Chunks.begin(); it != hn2Chunks.end(); ++it)
            {
                if((*it).compare((*itBis)) == 0)
                {
                    similarities++;
                    itBis++;
                }
                else
                    break;
            }
            
            if(similarities >= (size - 1))
                return true;
            return false;
        }
    }
    return false;
}

void AliasResolver::resolve(Neighborhood *n)
{
    this->discover(n->listAllInterfaces(), n->getAliases(), n->getFingerprints());
    this->postProcess(n->getAliases(), n->getSubnets());
}

list<Router*> AliasResolver::resolve(list<InetAddress> IPs, list<Fingerprint> *fingerprints)
{
    list<Router*> result;
    this->discover(IPs, &result, fingerprints, true);
    return result;
}

void AliasResolver::discover(list<InetAddress> interfaces, 
                             list<Router*> *results, 
                             list<Fingerprint> *fingerprintsList, 
                             bool strict)
{
    IPLookUpTable *table = env->getIPTable();
    
    // Remove duplicata (possible because ingress interface of neighborhood can be a contra-pivot)
    InetAddress previous(0);
    for(list<InetAddress>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        InetAddress current = (*i);

        if(current == previous)
            interfaces.erase(i--);
        
        previous = current;
    }
    
    // Evaluates IP ID counters of each IP and generates the fingerprints
    while(interfaces.size() > 0)
    {
        InetAddress cur = interfaces.front();
        interfaces.pop_front();
        
        IPTableEntry *entryCur = table->lookUp(cur);
        if(entryCur == NULL) // Skips if entry in IP dictionnary does not exist (not likely)
            continue;
        
        this->evaluateIPIDCounter(entryCur);
        
        Fingerprint curPrint(entryCur);
        fingerprintsList->push_back(curPrint);
    }
    fingerprintsList->sort(Fingerprint::compare);
    
    // Makes a copy of the sorted fingerprints
    list<Fingerprint> fingerprints((*fingerprintsList));
    
    // Starts association...
    while(fingerprints.size() > 0)
    {
        Fingerprint cur = fingerprints.front();
        fingerprints.pop_front();
        
        // No more fingerprints: creates a single router for cur (outside strict mode)
        if(fingerprints.size() == 0)
        {
            if(!strict)
            {
                Router *curRouter = new Router();
                curRouter->addInterface(InetAddress((InetAddress) (*cur.ipEntry)), 
                                        RouterInterface::FIRST_IP);
                results->push_back(curRouter);
            }
            break;
        }
        
        /* 
         * Lists similar fingerprints. Thanks to sorting, the similar fingerprints should occur 
         * directly after the current one.
         */
        
        list<Fingerprint> similar;
        Fingerprint next = fingerprints.front();
        while(fingerprints.size() > 0 && cur.equals(next))
        {
            similar.push_back(next);
            fingerprints.pop_front();
            
            if(fingerprints.size() > 0)
                next = fingerprints.front();
        }
        
        /*
         * If cur has its field "portUnreachableSrcIP" different from 0.0.0.0, we also look for 
         * a fingerprint which the IP is identical to the value of this field in "fingerprints" 
         * list. Indeed, the fingerprinting of such IP does not always work, for some reason.
         */
        
        if(cur.portUnreachableSrcIP != InetAddress(0))
        {
            for(list<Fingerprint>::iterator i = fingerprints.begin(); i != fingerprints.end(); ++i)
            {
                Fingerprint subCur = (*i);
                if((InetAddress) (*subCur.ipEntry) == cur.portUnreachableSrcIP)
                {
                    similar.push_back(subCur);
                    fingerprints.erase(i--);
                }
            }
        }
        
        if(similar.size() == 0)
        {
            /*
             * Case 1: there is no similar fingerprint in the list, therefore one creates a Router 
             * object for this IP only, except if features the data for the UDP port unreachable 
             * method. In this case, the fingerprint is edited to remove the port unreachable 
             * source IP, in order to try other aliasing methods with this fingerprint.
             */
            
            if(cur.portUnreachableSrcIP != InetAddress(0))
            {
                cur.portUnreachableSrcIP = InetAddress(0);
                fingerprints.push_front(cur);
                fingerprints.sort(Fingerprint::compare);
                continue;
            }
            else if(!strict) // Only created outside strict mode
            {
                Router *curRouter = new Router();
                curRouter->addInterface(InetAddress((InetAddress) (*cur.ipEntry)), 
                                        RouterInterface::FIRST_IP);
                results->push_back(curRouter);
            }
        }
        else if(cur.portUnreachableSrcIP != InetAddress(0))
        {
            /*
             * Case 2: similar fingerprints for which we have the data needed for the UDP 
             * port unreachable alias resolution method. Due to the sorting process and the usage 
             * of equals() while picking similar fingerprints (that is, the source IP in the Port 
             * Unreachable response is the same for all fingerprints), one can just create a 
             * single router with all the listed IPs and "cur".
             */
        
            Router *curRouter = new Router();
            curRouter->addInterface(InetAddress((InetAddress) (*cur.ipEntry)), 
                                    RouterInterface::FIRST_IP);
            
            while(similar.size() > 0)
            {
                Fingerprint subCur = similar.front();
                similar.pop_front();
                
                curRouter->addInterface(InetAddress((InetAddress) (*subCur.ipEntry)), 
                                        RouterInterface::UDP_PORT_UNREACHABLE);
            }
            
            results->push_back(curRouter);
        }
        else if(cur.IPIDCounterType == IPTableEntry::HEALTHY_COUNTER)
        {
            /*
             * Case 3: similar fingerprint and "healthy" counter: the Ally method is used.
             */
            
            Fingerprint ref = cur;
            list<Fingerprint> toProcess = similar;
            list<Fingerprint> excluded;
            list<Fingerprint> grouped;
            list<unsigned short> groupMethod;
            while(toProcess.size() > 0)
            {
                Fingerprint subCur = toProcess.front();
                toProcess.pop_front();
                
                // IP ID based alias resolution starts here
                grouped.push_front(ref);
                unsigned short AllyResult = this->groupAlly(subCur, grouped, env->getARAllyMaxDiff());
                grouped.pop_front();
                    
                // Ally method acknowledges the association
                if(AllyResult == ALLY_ACCEPTED)
                {
                    grouped.push_back(subCur);
                    groupMethod.push_back(RouterInterface::ALLY);
                }
                else
                    excluded.push_back(subCur);
                
                // When we are done with toProcess...
                if(toProcess.size() == 0)
                {
                    /*
                     * Checks for another router obtained through the address-based method with 
                     * "healthy" IPs, because we might miss an alias with some IPs which were 
                     * previously aliased through UDP unreachable port method.
                     */
                    
                    bool fusionOccurred = false;
                    for(list<Router*>::iterator it = results->begin(); it != results->end(); ++it)
                    {
                        Router *listed = (*it);

                        // Tries to alias with ref and a "merging pivot" (see Router.h)
                        IPTableEntry *mergingPivot = listed->getMergingPivot(table);
                        unsigned short fusionAliasMethod = 0;
                        if(mergingPivot != NULL)
                        {
                            unsigned short AllyResult = this->Ally(ref.ipEntry, 
                                                                   mergingPivot, 
                                                                   env->getARAllyMaxDiff());
                            
                            // Ally method acknowledges the association
                            if(AllyResult == ALLY_ACCEPTED)
                            {
                                fusionAliasMethod = RouterInterface::ALLY;
                                fusionOccurred = true;
                            }
                        }
                        
                        /*
                         * Fusion occurs here, because some weird stuff occurs if done outside the 
                         * loop: the "listed" variable disappears (because it is technically a 
                         * local variable), even if one use a pointer to it (with & operator), 
                         * causing the whole program to crash. We also have to delete "it" in the 
                         * results list and push the modified "listed" afterwards (yup, it's 
                         * twisted, but this is the limit of local variables in C++). The code 
                         * directly breaks out of the loop after the fusion.
                         */
                        
                        if(fusionOccurred)
                        {
                            results->erase(it--);
                            listed->addInterface(InetAddress((InetAddress) (*ref.ipEntry)), fusionAliasMethod);
                            
                            while(grouped.size() > 0)
                            {
                                Fingerprint h = grouped.front();
                                unsigned short aliasMethod = groupMethod.front();
                                grouped.pop_front();
                                groupMethod.pop_front();
                                
                                listed->addInterface(InetAddress((InetAddress) (*h.ipEntry)), aliasMethod);
                            }
                            
                            results->push_back(listed);
                            break;
                        }
                    }
                    
                    /*
                     * If no fusion occurred, creates a router with ref and IPs in grouped except 
                     * if grouped is empty and strict mode is enabled.
                     */
                    
                    if(!fusionOccurred && (!strict || grouped.size() > 0))
                    {
                        Router *curRouter = new Router();
                        curRouter->addInterface(InetAddress((InetAddress) (*ref.ipEntry)), 
                                                RouterInterface::FIRST_IP);
                        
                        while(grouped.size() > 0)
                        {
                            Fingerprint head = grouped.front();
                            unsigned short aliasMethod = groupMethod.front();
                            grouped.pop_front();
                            groupMethod.pop_front();
                            
                            curRouter->addInterface(InetAddress((InetAddress) (*head.ipEntry)), 
                                                    aliasMethod);
                        }
                        results->push_back(curRouter);
                    }
                    
                    // Takes care of excluded IPs
                    if(excluded.size() > 0)
                    {
                        ref = excluded.front();
                        excluded.pop_front();
                        
                        // If more than one excluded IP, updates toProcess and empties excluded
                        if(excluded.size() > 0)
                        {
                            while(excluded.size() > 0)
                            {
                                Fingerprint subSubCur = excluded.front();
                                excluded.pop_front();
                                toProcess.push_back(subSubCur);
                            }
                        }
                        // There remains a single router to create (only created if not strict)
                        else if(!strict)
                        {
                            Router *curRouter = new Router();
                            curRouter->addInterface(InetAddress((InetAddress) (*ref.ipEntry)), 
                                                    RouterInterface::FIRST_IP);
                            results->push_back(curRouter);
                        }
                    }
                }
            }
        }
        else if(!strict && cur.IPIDCounterType == IPTableEntry::FAST_COUNTER)
        {
           /*
            * Case 4 (never performed if strict mode): similar fingerprint and "fast" counter: 
            * the velocity-based method is used.
            */
          
            Fingerprint ref = cur;
            list<Fingerprint> toProcess = similar;
            list<Fingerprint> excluded;
            list<Fingerprint> grouped;
            list<unsigned short> groupMethod;
            while(toProcess.size() > 0)
            {
                Fingerprint subCur = toProcess.front();
                toProcess.pop_front();
                
                // Velocity-based alias resolution starts here
                grouped.push_front(ref);
                bool aliasable = this->groupVelocity(subCur, grouped);
                grouped.pop_front();
                
                if(aliasable)
                {
                    grouped.push_back(subCur);
                    groupMethod.push_back(RouterInterface::IPID_VELOCITY);
                }
                else
                    excluded.push_back(subCur);
                
                // When we are done with toProcess...
                if(toProcess.size() == 0)
                {
                    // Takes care of excluded IPs
                    if(excluded.size() > 0)
                    {
                        ref = excluded.front();
                        excluded.pop_front();
                        
                        // If more than one excluded IP, updates toProcess and empties excluded
                        if(excluded.size() > 0)
                        {
                            while(excluded.size() > 0)
                            {
                                Fingerprint subSubCur = excluded.front();
                                excluded.pop_front();
                                toProcess.push_back(subSubCur);
                            }
                        }
                        // There remains a single router to create (only created if not strict)
                        else if(!strict)
                        {
                            Router *curRouter = new Router();
                            curRouter->addInterface(InetAddress((InetAddress) (*ref.ipEntry)), 
                                                    RouterInterface::FIRST_IP);
                            results->push_back(curRouter);
                        }
                    }
                }
            }  
        }
        else if(!strict && cur.toGroupByDefault())
        {
            /*
             * Case 5 (never performed if strict mode): similar fingerprints and "group by 
             * default" policy (applied to echo and random counters): the fingerprints with DNS 
             * are double checked to exclude IPs with similar fingerprints but host names that do 
             * not match. The process is repeated as long as there are excluded IPs.
             *
             * If "cur" or the next fingerprint have no host name, then the whole group can be 
             * assumed to have not enough host names to perform reverse DNS (due to sorting).
             */
             
            if(!cur.hostName.empty() && !similar.front().hostName.empty())
            {
                Fingerprint ref = cur;
                list<Fingerprint> toProcess = similar;
                list<Fingerprint> excluded;
                list<Fingerprint> grouped;
                while(toProcess.size() > 0)
                {
                    Fingerprint subCur = toProcess.front();
                    toProcess.pop_front();
                    
                    if(!subCur.hostName.empty())
                    {
                        if(this->reverseDNS(ref.ipEntry, subCur.ipEntry))
                            grouped.push_back(subCur);
                        else
                            excluded.push_back(subCur);
                    }
                    else
                        grouped.push_back(subCur);
                    
                    if(toProcess.size() == 0)
                    {
                        // Creates the router with the IPs found in "grouped" + ref.
                        Router *curRouter = new Router();
                        curRouter->addInterface(InetAddress((InetAddress) (*ref.ipEntry)), 
                                                RouterInterface::FIRST_IP);
                        
                        while(grouped.size() > 0)
                        {
                            Fingerprint head = grouped.front();
                            InetAddress curInterface((InetAddress) (*head.ipEntry));
                            grouped.pop_front();
                            
                            // Alias method
                            unsigned short aliasMethod = RouterInterface::GROUP_ECHO_DNS;
                            if(head.IPIDCounterType == IPTableEntry::RANDOM_COUNTER)
                            {
                                if(!head.hostName.empty())
                                    aliasMethod = RouterInterface::GROUP_RANDOM_DNS;
                                else
                                    aliasMethod = RouterInterface::GROUP_RANDOM;
                            }
                            else
                            {
                                if(!head.hostName.empty())
                                    aliasMethod = RouterInterface::GROUP_ECHO_DNS;
                                else
                                    aliasMethod = RouterInterface::GROUP_ECHO;
                            }
                            
                            curRouter->addInterface(curInterface, aliasMethod);
                        }
                        
                        results->push_back(curRouter);
                        
                        // Takes care of excluded IPs
                        if(excluded.size() > 0)
                        {
                            ref = excluded.front();
                            excluded.pop_front();
                            
                            // Updates the list "toProcess" (others are already up to date)
                            if(excluded.size() > 0)
                            {
                                while(excluded.size() > 0)
                                {
                                    Fingerprint subSubCur = excluded.front();
                                    excluded.pop_front();
                                    toProcess.push_back(subSubCur);
                                }
                            }
                            // There remains a single router to create
                            else
                            {
                                Router *curRouter = new Router();
                                curRouter->addInterface(InetAddress((InetAddress) (*ref.ipEntry)), 
                                                       RouterInterface::FIRST_IP);
                                results->push_back(curRouter);
                            }
                        }
                    }
                }
            }
            else
            {
                // Single router made of cur and every IP listed in "similar"
                Router *curRouter = new Router();
                curRouter->addInterface(InetAddress((InetAddress) (*cur.ipEntry)), 
                                        RouterInterface::FIRST_IP);
                
                unsigned short aliasMethod = RouterInterface::GROUP_ECHO;
                if(cur.IPIDCounterType == IPTableEntry::RANDOM_COUNTER)
                    aliasMethod = RouterInterface::GROUP_RANDOM;
                
                while(similar.size() > 0)
                {
                    Fingerprint subCur = similar.front();
                    similar.pop_front();
                    
                    curRouter->addInterface(InetAddress((InetAddress) (*subCur.ipEntry)), 
                                            aliasMethod);
                }
                
                results->push_back(curRouter);
            }
        }
        else if(!strict && cur.IPIDCounterType == IPTableEntry::NO_IDEA)
        {
            /*
             * Case 6 (never performed if strict mode): case of IPs for which no IP-ID data could 
             * be collected. There are two possibilites:
             * -either there is no host name (easily checked with cur, since fingerprints with 
             *  host names should come first), then a single router is created for each IP,
             * -either there are host names, in which case one separates the IPs with host names 
             *  from the others (for those, it is the same outcome as in previous point) and 
             *  attempts gathering them through reverse DNS.
             */
            
            // First takes care of IPs with no data at all
            if(cur.hostName.empty())
            {
                similar.push_front(cur);
                while(similar.size() > 0)
                {
                    Fingerprint subCur = similar.front();
                    similar.pop_front();
                    
                    Router *curRouter = new Router();
                    curRouter->addInterface(InetAddress((InetAddress) (*subCur.ipEntry)), 
                                            RouterInterface::FIRST_IP);
                    results->push_back(curRouter);
                }
            }
            else
            {
                for(list<Fingerprint>::iterator it = similar.begin(); it != similar.end(); ++it)
                {
                    Fingerprint subCur = (*it);
                    if(subCur.hostName.empty())
                    {
                        Router *curRouter = new Router();
                        curRouter->addInterface(InetAddress((InetAddress) (*subCur.ipEntry)), 
                                                RouterInterface::FIRST_IP);
                        results->push_back(curRouter);
                        
                        similar.erase(it--);
                    }
                }
            }
            
            // If similar is empty, there remains "cur": creates a router for it
            if(similar.size() == 0)
            {
                Router *curRouter = new Router();
                curRouter->addInterface(InetAddress((InetAddress) (*cur.ipEntry)), 
                                        RouterInterface::FIRST_IP);
                results->push_back(curRouter);
                continue;
            }
            
            // Then takes care of those with a known host name
            Fingerprint ref = cur;
            list<Fingerprint> toProcess = similar;
            list<Fingerprint> excluded;
            list<Fingerprint> grouped;
            while(toProcess.size() > 0)
            {
                Fingerprint subCur = toProcess.front();
                toProcess.pop_front();
                
                if(this->reverseDNS(ref.ipEntry, subCur.ipEntry))
                    grouped.push_back(subCur);
                else
                    excluded.push_back(subCur);
                
                if(toProcess.size() == 0)
                {
                    // Creates a router with ref and IPs in grouped
                    Router *curRouter = new Router();
                    curRouter->addInterface(InetAddress((InetAddress) (*ref.ipEntry)), 
                                            RouterInterface::FIRST_IP);
                    
                    while(grouped.size() > 0)
                    {
                        Fingerprint head = grouped.front();
                        grouped.pop_front();
                        
                        curRouter->addInterface(InetAddress((InetAddress) (*head.ipEntry)), 
                                                RouterInterface::REVERSE_DNS);
                    }
                    results->push_back(curRouter);
                    
                    // Takes care of excluded IPs
                    if(excluded.size() > 0)
                    {
                        ref = excluded.front();
                        excluded.pop_front();
                        
                        // If more than 1 excluded IPs, updates toProcess and empties excluded
                        if(excluded.size() > 0)
                        {
                            while(excluded.size() > 0)
                            {
                                Fingerprint subSubCur = excluded.front();
                                excluded.pop_front();
                                toProcess.push_back(subSubCur);
                            }
                        }
                        // There remains a single router to create
                        else
                        {
                            Router *curRouter = new Router();
                            curRouter->addInterface(InetAddress((InetAddress) (*ref.ipEntry)), 
                                                    RouterInterface::FIRST_IP);
                            results->push_back(curRouter);
                        }
                    }
                }
            }
        }
        else
        {
            /*
             * Case 7: if we are in "strict" mode and reach this block, nothing happens. We want 
             * to only produce Router objects from UDP-based method and Ally method with at least 
             * two interfaces per object.
             */
        }
    }
}

void AliasResolver::postProcess(list<Router*> *results, list<SubnetSite*> *subnets)
{
    /*
     * Removes the routers consisting of a single interface which happens to be a candidate 
     * contra-pivot of an ODD subnet, except if this is a neighborhood label.
     *
     * N.B.: checking the interface appears in the subnet responsive IPs list is enough, as the 
     * pivots were not listed at all in the potential interfaces.
     */
     
    for(list<Router*>::iterator i = results->begin(); i != results->end(); ++i)
    {
        if((*i)->getNbInterfaces() == 1)
        {
            InetAddress interface = (*i)->getInterfacesList()->front()->ip;
            
            bool inOddSubnet = false, neighborhoodLabel = false;
            for(list<SubnetSite*>::iterator j = subnets->begin(); j != subnets->end(); ++j)
            {
                SubnetSite *ss = (*j);
                if(ss->getStatus() == SubnetSite::ODD_SUBNET && ss->getNode(interface) != NULL)
                    inOddSubnet = true;
                if(ss->getNeighborhoodLabelAnomalies() == 0 && ss->getNeighborhoodLabelIP() == interface)
                    neighborhoodLabel = true;
            }
            
            if(inOddSubnet && !neighborhoodLabel)
            {
                delete (*i);
                results->erase(i--);
            }
        }
    }
    
    /*
     * Then, the list is sorted and duplicates are removed. Duplicates are a very rare occurrence 
     * but can happen when the contra-pivot of a subnet and the neighborhood label IP for this 
     * subnet are the same IP (a consequence of a specific routing policy or a bad measurement).
     */
    
    results->sort(Router::compare);
    Router *previousRouter = NULL;
    for(list<Router*>::iterator i = results->begin(); i != results->end(); ++i)
    {
        Router *cur = (*i);
        
        if(previousRouter != NULL)
        {
            if(cur->equals(previousRouter))
            {
                delete cur;
                results->erase(i--);
            }
            else
            {
                previousRouter = cur;
            }
        }
        else
        {
            previousRouter = cur;
        }
    }
}
