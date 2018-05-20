/*
 * IPLookUpTable.h
 *
 *  Created on: Sep 29, 2015
 *      Author: jefgrailet
 *
 * IPLookUpTable is a particular structure built to quickly look up for an IP address and obtain 
 * an object storing data about it, such as alias resolution hints or the timeout value that 
 * should be preferred when probing this IP. The structure should only contain IPs discovered to 
 * be responsive at pre-scanning.
 *
 * The structure is made of an array of lists which are indexed from 0 to 2^X. An IP is stored in 
 * the structure by adding it to the list at the index which matches its X first bits. This 
 * implies that each list can contain at most 2^(32 - X) items, thanks to which the storage and 
 * look-up can be achieved in O(1) as long as X is great enough.
 *
 * The choice of X is theoretically free. For now, it is set to 20, which leads to a 8 Mio array 
 * of lists where each lists can contain up to 4096 elements. Using as much memory for the array is 
 * still reasonable and should not bother any of today's computers.
 */

#ifndef IPLOOKUPTABLE_H_
#define IPLOOKUPTABLE_H_

#include <list>
using std::list;

#include "IPTableEntry.h"

class IPLookUpTable
{
public:
    
    // X = 20
    const static unsigned int SIZE_TABLE = 1048576;
    
    // Constructor, destructor
    IPLookUpTable(unsigned short nbIPIDs);
    ~IPLookUpTable();
    
    // Methods to check if the dictionnary is empty or not and count the IPs it contains
    bool isEmpty();
    unsigned int getTotalIPs();
    
    // Creation and look-up methods
    IPTableEntry *create(InetAddress needle); // NULL if already existed
    IPTableEntry *lookUp(InetAddress needle); // NULL if not found
    
    // Output methods
    void outputDictionnary(string filename);
    void outputFingerprints(string filename);
    
private:
    
    list<IPTableEntry*> *haystack;
    unsigned short nbIPIDs;
};

#endif /* IPLOOKUPTABLE_H_ */
