/*
 * DirectLink.h
 *
 *  Created on: Nov 13, 2017
 *      Author: jefgrailet
 *
 * Models a direct link. A direct link, in this situation, denotes an edge between two consecutive 
 * neighborhoods, with the first neighborhood containing a subnet encompassing a label (= route 
 * hop) of the second. It is analoguous to the "on the way" methodology in TreeNET, and as such, 
 * adds a pointer to a SubnetSite object w.r.t. Edge.
 */

#ifndef DIRECTLINK_H_
#define DIRECTLINK_H_

#include "Edge.h"

class DirectLink : public Edge
{
public:

    DirectLink(Neighborhood *tail, Neighborhood *head, SubnetSite *medium);
    ~DirectLink();
    
    string toString();
    string toStringVerbose();

protected:
    
    SubnetSite *medium;
    
};

#endif /* DIRECTLINK_H_ */
