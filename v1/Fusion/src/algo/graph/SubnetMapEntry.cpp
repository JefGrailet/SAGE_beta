/*
 * SubnetMapEntry.cpp
 *
 *  Created on: Oct 4, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetMapEntry.h (see this file to learn further about the 
 * goals of such class).
 */

#include "SubnetMapEntry.h"

SubnetMapEntry::SubnetMapEntry(SubnetSite *subnet, Neighborhood *n)
{
    this->subnet = subnet;
    this->n = n;
}

SubnetMapEntry::~SubnetMapEntry()
{
}

bool SubnetMapEntry::compare(SubnetMapEntry *sme1, SubnetMapEntry *sme2)
{
    return SubnetSite::compare(sme1->subnet, sme2->subnet);
}
