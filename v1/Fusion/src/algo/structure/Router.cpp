/*
 * Router.cpp
 *
 *  Created on: Mar 1, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in Router.h (see this file to learn further about the goals of 
 * such a class).
 */

#include <string>
using std::string;

#include "Router.h"

using namespace std;

Router::Router()
{
}

Router::~Router()
{
    for(list<RouterInterface*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        delete (*i);
    }
}

void Router::addInterface(InetAddress interface, unsigned short aliasMethod)
{
    RouterInterface *newInterface = new RouterInterface(interface, aliasMethod);
    interfaces.push_back(newInterface);
    interfaces.sort(RouterInterface::smaller);
}

unsigned short Router::getNbInterfaces()
{
    return (unsigned short) interfaces.size();
}

bool Router::hasInterface(InetAddress interface)
{
    for(list<RouterInterface*>::iterator i = interfaces.begin(); i != interfaces.end(); ++i)
    {
        if((*i)->ip == interface)
        {
            return true;
        }
    }
    return false;
}

IPTableEntry* Router::getMergingPivot(IPLookUpTable *table)
{
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        RouterInterface *interface = (*it);
        if(interface->aliasMethod == RouterInterface::UDP_PORT_UNREACHABLE)
        {
            IPTableEntry *entry = table->lookUp(interface->ip);
            if(entry != NULL && entry->getIPIDCounterType() == IPTableEntry::HEALTHY_COUNTER)
                return entry;
        }
    }
    return NULL;
}

string Router::toString()
{
    stringstream result;
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << " ";
        result << (*it)->ip;
    }
    return result.str();
}

string Router::toStringSemiVerbose()
{
    stringstream result;
    unsigned short aliasMethods[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << ", ";
        result << (*it)->ip;
        if((*it)->aliasMethod != RouterInterface::FIRST_IP)
        {
            switch((*it)->aliasMethod)
            {
                case RouterInterface::UDP_PORT_UNREACHABLE:
                    aliasMethods[0]++;
                    break;
                case RouterInterface::ALLY:
                    aliasMethods[1]++;
                    break;
                case RouterInterface::IPID_VELOCITY:
                    aliasMethods[2]++;
                    break;
                case RouterInterface::REVERSE_DNS:
                    aliasMethods[3]++;
                    break;
                case RouterInterface::GROUP_ECHO:
                    aliasMethods[4]++;
                    break;
                case RouterInterface::GROUP_ECHO_DNS:
                    aliasMethods[5]++;
                    break;
                case RouterInterface::GROUP_RANDOM:
                    aliasMethods[6]++;
                    break;
                case RouterInterface::GROUP_RANDOM_DNS:
                    aliasMethods[7]++;
                    break;
                default:
                    aliasMethods[8]++;
                    break;
            }
        }
    }
    
    unsigned short maximum = aliasMethods[0], maxIndex = 0, total = aliasMethods[0];
    for(unsigned short i = 1; i < 9; i++)
    {
        if(aliasMethods[i] > maximum)
        {
            maximum = aliasMethods[i];
            maxIndex = i;
        }
        total += aliasMethods[i];
    }
    
    if(total > 0)
    {
        string mainMethod = "";
        switch(maxIndex)
        {
            case 0:
                mainMethod = "UDP-based";
                break;
            case 1:
                mainMethod = "Ally";
                break;
            case 2:
                mainMethod = "IP-ID velocity";
                break;
            case 3:
                mainMethod = "reverse DNS";
                break;
            case 4:
                mainMethod = "group by echo IP-ID counter";
                break;
            case 5:
                mainMethod = "group by echo IP-ID counter/DNS";
                break;
            case 6:
                mainMethod = "group by random IP-ID counter";
                break;
            case 7:
                mainMethod = "group by random IP-ID counter/DNS";
                break;
            default:
                mainMethod = "Unknown";
                break;
        }
    
        if(total == maximum)
        {
            result << " (" << mainMethod << ")";
        }
        else
        {
            float ratio = ((float) maximum / (float) total) * 100;
            result << " (" << mainMethod << ", " << ratio << "%" << " of aliases)";
        }
    }
    
    return result.str();
}

string Router::toStringVerbose()
{
    stringstream result;
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << ", ";
        result << (*it)->ip;
        if((*it)->aliasMethod != RouterInterface::FIRST_IP)
        {
            result << " (";
            switch((*it)->aliasMethod)
            {
                case RouterInterface::UDP_PORT_UNREACHABLE:
                    result << "UDP unreachable port";
                    break;
                case RouterInterface::ALLY:
                    result << "Ally";
                    break;
                case RouterInterface::IPID_VELOCITY:
                    result << "IP-ID Velocity";
                    break;
                case RouterInterface::REVERSE_DNS:
                    result << "Reverse DNS";
                    break;
                case RouterInterface::GROUP_ECHO:
                    result << "Echo group";
                    break;
                case RouterInterface::GROUP_ECHO_DNS:
                    result << "Echo group & DNS";
                    break;
                case RouterInterface::GROUP_RANDOM:
                    result << "Random group";
                    break;
                case RouterInterface::GROUP_RANDOM_DNS:
                    result << "Random group & DNS";
                    break;
                default:
                    break;
            }
            result << ")";
        }
    }
    return result.str();
}

string Router::toStringMinimalist()
{
    stringstream result;
    result << "[";
    bool shortened = false;
    unsigned short i = 0;
    for(list<RouterInterface*>::iterator it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        if(it != interfaces.begin())
            result << ", ";
        
        if(i == 3)
        {
            shortened = true;
            result << "...";
            break;
        }
        else
            result << (*it)->ip;
        i++;
    }
    result << "]";
    if(shortened)
    {
        result << " (" << interfaces.size() << " interfaces)";
    }
    return result.str();
}

bool Router::compare(Router *r1, Router *r2)
{
    unsigned short size1 = r1->getNbInterfaces();
    unsigned short size2 = r2->getNbInterfaces();
    
    bool result = false;
    if (size1 < size2)
    {
        result = true;
    }
    else if(size1 == size2)
    {
        list<RouterInterface*> *list1 = r1->getInterfacesList();
        list<RouterInterface*> *list2 = r2->getInterfacesList();
        
        list<RouterInterface*>::iterator it2 = list2->begin();
        for(list<RouterInterface*>::iterator it1 = list1->begin(); it1 != list1->end(); it1++)
        {
            InetAddress ip1 = (*it1)->ip;
            InetAddress ip2 = (*it2)->ip;
            
            if(ip1 < ip2)
            {
                result = true;
                break;
            }
            else if(ip1 > ip2)
            {
                result = false;
                break;
            }
        
            it2++;
        }
    }
    return result;
}

bool Router::compareBis(Router *r1, Router *r2)
{
    RouterInterface *ri1 = r1->getInterfacesList()->front();
    RouterInterface *ri2 = r2->getInterfacesList()->front();
    if(ri1->ip < ri2->ip)
        return true;
    return false;
}

bool Router::equals(Router *other)
{
    unsigned short size1 = (unsigned short) interfaces.size();
    unsigned short size2 = other->getNbInterfaces();
    
    if(size1 == size2)
    {
        list<RouterInterface*> *list2 = other->getInterfacesList();
        list<RouterInterface*>::iterator it2 = list2->begin();
        for(list<RouterInterface*>::iterator it1 = interfaces.begin(); it1 != interfaces.end(); it1++)
        {
            InetAddress ip1 = (*it1)->ip;
            InetAddress ip2 = (*it2)->ip;
            
            if(ip1 < ip2 || ip1 > ip2)
                return false;
        
            it2++;
        }
        
        return true;
    }
    return false;
}
