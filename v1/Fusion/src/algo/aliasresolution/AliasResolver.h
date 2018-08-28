/*
 * AliasResolver.h
 *
 *  Created on: Oct 20, 2015
 *      Author: jefgrailet
 *
 * In order to thoroughly refactor the alias resolution module of TreeNET, previous AliasResolver 
 * has been renamed AliasHintCollector (a more fitting name), while this new class indeed performs 
 * alias resolution by using the "hints" collected by the former class. The goal of having a class 
 * for this is to separate the different alias resolution techniques from other parts of the code 
 * where they do not really belong. For example, the actual router inference in TreeNET v1.0 took 
 * place in NetworkTreeNode class, which is not what this class was built for at first.
 *
 * Over time, AliasResolver has been progressively extended to implement several state-of-the-art 
 * alias resolution techniques and select the most suited method for a given set of IPs, using 
 * IP fingerprinting. It could be easily re-used from TreeNET with some light modifications, as 
 * the only particular components of TreeNET it dealt with initially were NetworkTreeNode objects, 
 * now replaced by Neighborhood objects from src/algo/graph/.
 */

#ifndef ALIASRESOLVER_H_
#define ALIASRESOLVER_H_

#include "../Environment.h"
#include "../structure/Router.h"
#include "../graph/components/Neighborhood.h"
#include "Fingerprint.h"

class AliasResolver
{
public:

    // Possible results for Ally method
    enum AllyResults
    {
        ALLY_NO_SEQUENCE, // A token sequence could not be found
        ALLY_REJECTED, // The sequence exists, but Ally rejects it
        ALLY_ACCEPTED // The sequences exists and Ally acknowledges the association
    };

    // Constructor/destructor
    AliasResolver(Environment *env);
    ~AliasResolver();
    
    // Starts resolving a Neighborhood
    void resolve(Neighborhood *n);
    
    /*
     * Resolves a list of InetAddress objects (during graph building only). Aliases are returned, 
     * while fingerprints are obtained via a pointer. For now, this method always uses the strict 
     * mode of discover().
     *
     * @param list<InetAddress>  IPs           The IPs on which alias resolution must be conducted
     * @param list<Fingerprint>* fingerprints  The list where fingerprints will be found after
     * @return list<Router*>                   The obtained aliases
     */
    
    list<Router*> resolve(list<InetAddress> IPs, list<Fingerprint> *fingerprints);
    
private:

    // Pointer to the environment object (=> IP table)
    Environment *env;
    
    /*
     * Method to perform Ally method for alias resolution, i.e., if for 2 distinct IPs, we have
     * 3 IP IDs with increasing tokens which are also increasing themselves and close in values, 
     * 2 of them being from the first IP and the last one from the second, they are likely from 
     * the same router.
     *
     * @param IPTableEntry* ip1       The first IP to associate
     * @param IPTableEntry* ip2       The second IP to associate
     * @param unsigned short maxDiff  The largest gap to be accepted between 2 consecutive IDs
     * @return unsigned short         One of the results listed in "enum AllyResults"
     */
    
    unsigned short Ally(IPTableEntry *ip1, IPTableEntry *ip2, unsigned short maxDiff);
    
    /* 
     * Method to perform Ally method between each member of a group of fingerprints and a given, 
     * isolated fingerprint. The purpose of this method is to check that all fingerprints grouped 
     * for a growing alias are compatible with the additionnal fingerprint (there must be no 
     * possible rejection by Ally).
     * 
     * @param Fingerprint isolatedIP    The isolated IP
     * @param list<Fingerprint> group   The group of already aliased IPs
     * @param unsigned short maxDiff    The largest accepted gap (just like for Ally())
     * @return unsigned short           One of the results listed in "enum AllyResults"
     */
    
    unsigned short groupAlly(Fingerprint isolatedIP, 
                             list<Fingerprint> group, 
                             unsigned short maxDiff);
    
    /*
     * Method to evaluate an IP ID counter, which amounts to computing its velocity and/or 
     * labelling it with a (unsigned short) class defined in IPTableEntry.h. It returns nothing.
     *
     * @param IPTableEntry* ip  The IP for which the IP ID counter needs to be evaluated
     */
    
    void evaluateIPIDCounter(IPTableEntry *ip);
    
    /*
     * Method to check if the velocity ranges of two distinct IPs (given as IPTableEntry objects) 
     * overlap, if which case both IPs should be associated.
     *
     * @param IPTableEntry* ip1  The first IP to associate
     * @param IPTableEntry* ip2  The second IP to associate
     * @return bool              True if both ranges overlap, false if not or if one or both IPs 
     *                           do not have a velocity range
     */
    
    bool velocityOverlap(IPTableEntry *ip1, IPTableEntry *ip2);
    
    /* 
     * Just like groupAlly(), next method checks an isolated IP is compatible with a growing alias 
     * of IPs associated with the velocity-based approach. The purpose is to check that all 
     * fingerprints grouped in the growing alias are compatible with the additionnal fingerprint 
     * velocity-wise.
     * 
     * @param Fingerprint isolatedIP    The isolated IP
     * @param list<Fingerprint> group   The group of already aliased IPs
     * @return unsigned short           True if the isolated IP is compatible
     */
    
    bool groupVelocity(Fingerprint isolatedIP, list<Fingerprint> group);
    
    /*
     * Method to check if two distinct IPs (given as IPTableEntry objects) can be associated 
     * with the reverse DNS method.
     *
     * @param IPTableEntry* ip1  The first IP to associate
     * @param IPTableEntry* ip2  The second IP to associate
     * @return bool              True if association is possible, false otherwise
     */
    
    bool reverseDNS(IPTableEntry *ip1, IPTableEntry *ip2);
    
    /*
     * Method to apply alias resolution to a group of IPs, modeled by a list of IPs provided along 
     * lists of Router objects and Fingerprint objects. These lists will contain the resulting 
     * Router objects and the fingerprints that were derived for each IP.
     *
     * A fourth optional parameter (set by default to false) can be set to true to enable "strict" 
     * mode. In this mode, discover() behaves the same way but skips some alias resolution methods 
     * that are considered too optimistic (such as group by fingerprint, unless DNS does not match 
     * at all). It also avoids creating Router objects consisting of a single interface.
     *
     * @param list<InetAddress>   interfaces    The IPs to alias
     * @param list<Router*>*      results       The list that will contain resulting Router objects
     * @param list<Fingerprint>*  fingerprints  The fingerprints derived for each IP in the process
     * @param bool                strict        To set to True if we want to only use IP-ID-based 
     *                                          and UDP-based methods, i.e., strict mode
     */
    
    void discover(list<InetAddress> interfaces, 
                  list<Router*> *results, 
                  list<Fingerprint> *fingerprints, 
                  bool strict = false);
    
    /*
     * Method to post-process the aliases discovered on a group of interfaces which a part of 
     * belong to subnets. The motivation for this post-processing is that some IPs identified as 
     * router interfaces in subnets might just be outliers. Therefore, when such interfaces appear 
     * in the results as aliases consisting of a single IP, they should be removed.
     *
     * In practice: if the interface of a single interface alias appears in an ODD subnet and is 
     * not a penultimate hop of one of the listed subnets, it should be removed. 
     *
     * @param list<Router*>*      results   The list with the aliases to post-process
     * @param list<SubnetSite*>*  subnets   The list of subnets which should be checked to remove 
     *                                      single interface aliases
     */
    
    void postProcess(list<Router*> *results, list<SubnetSite*> *subnets);
    
};

#endif /* ALIASRESOLVER_H_ */
