/*
 * SubnetSiteNode.cpp
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * Edited by J.-F. Grailet (October 2014) until and adapted for some versions of TreeNET then 
 * re-used in SAGE.
 */

#include "SubnetSiteNode.h"

SubnetSiteNode::SubnetSiteNode(const InetAddress &i, unsigned char T):
ip(i),
TTL(T)
{
}

SubnetSiteNode::~SubnetSiteNode() {}
