/*
 * RemoteLink.cpp
 *
 *  Created on: Nov 14, 2017
 *      Author: jefgrailet
 *
 * Implements the class defined in RemoteLink.h (see this file to learn further about the goals of 
 * such a class).
 */

#include "RemoteLink.h"

RemoteLink::RemoteLink(Neighborhood *tail, 
                       Neighborhood *head, 
                       list<MiscHop*> media) : Edge(Edge::REMOTE_LINK, tail, head)
{
    this->media = media;
}

RemoteLink::RemoteLink(Neighborhood *tail, 
                       Neighborhood *head) : Edge(Edge::REMOTE_LINK, tail, head)
{
}

RemoteLink::~RemoteLink()
{
}

string RemoteLink::toString()
{
    stringstream ss;
    
    ss << "N" << tail->getID() << " -> N" << head->getID();
    if(media.size() > 0)
    {
        if(media.size() > 1)
            ss << " via miscellaneous hops ";
        else
            ss << " via miscellaneous hop ";
        for(list<MiscHop*>::iterator i = media.begin(); i != media.end(); ++i)
        {
            if(i != media.begin())
                ss << ", ";
            ss << (*i)->getIP();
        }
    }
    else
        ss << " via anonymous path";
    
    return ss.str();
}

string RemoteLink::toStringVerbose()
{
    stringstream ss;
    
    ss << "Neighborhood " << tail->getFullLabel() << " to neighborhood " << head->getFullLabel();
    if(media.size() > 0)
    {
        if(media.size() == 1)
        {
            ss << " via a miscellaneous path starting at " << media.front()->getIP();
        }
        else
        {
            ss << " via miscellaneous paths starting at ";
            for(list<MiscHop*>::iterator i = media.begin(); i != media.end(); ++i)
            {
                if(i != media.begin())
                    ss << ", ";
                ss << (*i)->getIP();
            }
        }
    }
    else
        ss << " via a hidden path (i.e., only anonymous hops)";
    
    return ss.str();
}
