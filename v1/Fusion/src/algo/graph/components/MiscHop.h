/*
 * MiscHop.h
 *
 *  Created on: Nov 30, 2017
 *      Author: jefgrailet
 *
 * Models a "miscellaneous hop", i.e., an intermediate step in a path between two neighborhoods 
 * (or more) connected with a RemoteLink object. Such a hop is equivalent to a traceroute hop 
 * which does not appear elsewhere as the neighborhood label of some neighborhood. It is directly 
 * connected to other MiscHop objects, and edges are only "logical" in this situation.
 */

#ifndef MISCHOP_H_
#define MISCHOP_H_

#include "Neighborhood.h"

class MiscHop
{
public:

    MiscHop(InetAddress IP); // Regular hop obtained via traceroute
    MiscHop(unsigned short TTL); // Anonymous hop (could not be obtained with traceroute)
    ~MiscHop();
    
    // Accessers
    inline InetAddress getIP() { return this->IP; }
    inline unsigned short getTTL() { return this->TTL; }
    
    static bool compare(MiscHop *s1, MiscHop *s2);
    
    // Handles connections
    void connectTo(MiscHop *nextHop);
    void connectTo(Neighborhood *exit);
    
    string toString();

protected:
    
    InetAddress IP;
    unsigned short TTL; // Set to a value > 0 if IP amounts to a missing hop (i.e., = 0.0.0.0)
    
    /*
     * Important note: TTL value is solely used to not mix anonymous hop observed at different 
     * TTLs together. Say we have this route:
     * ..., 1.2.3.4, 0.0.0.0, 0.0.0.0, 5.6.7.8
     * Then using a same node for all 0.0.0.0 would amount to having a cycle or more on its 
     * object equivalent, while there is no guarantee such a cycle exist. For regular IPs, TTL is 
     * not set because issues like route stretching can cause a same IP to occur at different 
     * TTLs, and we don't want to distinguish these cases because of the TTL.
     */
    
    list<MiscHop*> nextHops;
    list<Neighborhood*> exits;
    
};

#endif /* MISCHOP_H_ */
