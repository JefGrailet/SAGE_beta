/*
 * SubnetSiteNode.h
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Edited by J.-F. Grailet (october 2014) to improve coding style and study the code.
 */

#ifndef SUBNETSITENODE_H_
#define SUBNETSITENODE_H_

#include <iostream>
using std::ostream;

#include "../../common/inet/InetAddress.h"

class SubnetSiteNode
{
public:

    // Overriden << operator, only used by debug mode (see also SubnetSite.h)
    friend ostream &operator<<(ostream &out, const SubnetSiteNode &ssn)
    {
        out << "{"
        << *(ssn.ip.getBinaryRepresentation())
        << "   "
        << *(ssn.ip.getHumanReadableRepresentation())
        << " - "
        << (int) ssn.TTL
        << " @ /"
        << (int) ssn.prefix
        << "  "
        << SubnetSiteNode::subnetSiteNodeStatusToString(ssn.nodeStatus)
        << "  "
        << SubnetSiteNode::subnetSiteAliasDiscoveryMethodToString(ssn.aliasStatus)
        << "}";
        return out;
    }

    enum SubnetSiteNodeStatus
    {
        MARKED_AS_INSIDE_SUBNET_BOUNDARIES = 1,
        MARKED_FOR_REMOVAL_DUE_TO_SUBNET_OVERGROWTH = 2,
        MARKED_FOR_REMOVAL_DUE_TO_BOUNDARY_ADDRESS_INCOMPATIBILITY = 4 // Broadcast or network address usage
    };

    enum AliasDiscoveryMethod
    {
        SOURCE_BASED_ALIAS_SX,
        DISTANCE_BASED_ALIAS_SX,
        MATE3031_BASED_ALIAS_SX,
        PALMTREE_BASED_ALIAS_SX,
        UNKNOWN_ALIAS_SX
    };

    static string subnetSiteAliasDiscoveryMethodToString(enum SubnetSiteNode::AliasDiscoveryMethod method);
    static string subnetSiteNodeStatusToString(enum SubnetSiteNode::SubnetSiteNodeStatus nodeStatus);
    
    // Comparison method for sorting purposes
    inline static bool smaller(SubnetSiteNode *ssn1, SubnetSiteNode *ssn2) { return ssn1->ip < ssn2->ip; }

    // Constructor, destructor
    SubnetSiteNode(const InetAddress &ip, 
                   unsigned char prefix, 
                   unsigned char TTL, 
                   enum SubnetSiteNode::AliasDiscoveryMethod aliasStatus, 
                   bool addedAtFilling = false);
    virtual ~SubnetSiteNode();
    
    // Fields
    InetAddress ip;
    unsigned char prefix;
    unsigned char TTL;
    enum SubnetSiteNode::AliasDiscoveryMethod aliasStatus; // Contrapivot
    enum SubnetSiteNode::SubnetSiteNodeStatus nodeStatus;
    bool addedAtFilling; // See below (April 2017)
    
    /* 
     * About "addedAtFilling": IPs added during the filling are most of the time IPs which were 
     * responsive during pre-scanning but not during scanning, hence, they were classed as Pivot 
     * IPs (because it is the best we can infer in this case) but should be avoided as targets 
     * during the traceroute phase. Hence this special field.
     */
    
};

#endif /* SUBNETSITENODE_H_ */