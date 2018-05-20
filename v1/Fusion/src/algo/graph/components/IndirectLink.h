/*
 * IndirectLink.h
 *
 *  Created on: Nov 13, 2017
 *      Author: jefgrailet
 *
 * Models an indirect link. An indirect link is a link between two consecutive neighborhoods for 
 * which the medium (i.e., the subnet encompassing a label of the second neighborhood) either 
 * could not be found, either is located remotely - that is, in the vicinity of a neighborhood 
 * other than the first neighborhood. The medium is this time modeled by a pointer to a 
 * SubnetMapEntry object (set to NULL if no medium).
 */

#ifndef INDIRECTLINK_H_
#define INDIRECTLINK_H_

#include "Edge.h"
#include "../SubnetMapEntry.h"

class IndirectLink : public Edge
{
public:

    IndirectLink(Neighborhood *tail, Neighborhood *head, SubnetMapEntry *medium);
    IndirectLink(Neighborhood *tail, Neighborhood *head); // To use when no medium could be found
    ~IndirectLink();

    inline bool hasMedium() { return medium != NULL; }
    
    string toString();
    string toStringVerbose();

protected:
    
    SubnetMapEntry *medium;
    
};

#endif /* INDIRECTLINK_H_ */
