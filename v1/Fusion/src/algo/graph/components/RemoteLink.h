/*
 * RemoteLink.h
 *
 *  Created on: Nov 14, 2017
 *      Author: jefgrailet
 *
 * Models a remote link. A remote link is a link between two neighborhoods for which there exist 
 * intermediate steps that do not appear as neighborhood labels. Though the edge directly connects 
 * both objects, it is more about giving directions, though an additionnal field leads to a 
 * sub-graph of traceroute hops which are handled simplier than the main graph, except for when 
 * the intermediate steps exclusively consist of anonymous hops.
 */

#ifndef REMOTELINK_H_
#define REMOTELINK_H_

#include "Edge.h"
#include "MiscHop.h"

class RemoteLink : public Edge
{
public:

    RemoteLink(Neighborhood *tail, Neighborhood *head, list<MiscHop*> media);
    RemoteLink(Neighborhood *tail, Neighborhood *head); // For exclusively anonymous path
    ~RemoteLink();

    string toString();
    string toStringVerbose();

protected:

    list<MiscHop*> media;

};

#endif /* REMOTELINK_H_ */
