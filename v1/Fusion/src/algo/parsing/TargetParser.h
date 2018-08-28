/*
 * TargetParser.h
 *
 *  Created on: Oct 2, 2015
 *      Author: jefgrailet
 *
 * This class is dedicated to parsing the target IPs and IP blocks which are provided at launch. 
 * Initially, the whole code was found in Main.cpp, but its size and the need to handle the 
 * listing of pre-scanning targets in addition to the usual targets led to the creation of this 
 * class. In addition to parsing, this class also handles the target re-ordering.
 *
 * N.B.: this version is slightly different than the one in "Discovery", because it is only used 
 * to get and verify IPv4 prefixes/addresses and storing them in the Environment singleton as the 
 * initial targets.
 */
 
#ifndef TARGETPARSER_H_
#define TARGETPARSER_H_

#include <ostream>
using std::ostream;
#include <string>
using std::string;
#include <list>
using std::list;

#include "../Environment.h"
#include "../../common/inet/NetworkAddress.h"

class TargetParser
{
public:

    static const unsigned int MIN_LENGTH_TARGET_STR = 6;

    // Constructor, destructor
    TargetParser(Environment *env);
    ~TargetParser();
    
    void parseCommandLine(string targetListStr);
    
    // Accessers to parsed elements
    inline list<InetAddress> getParsedIPs() { return this->parsedIPs; }
    inline list<NetworkAddress> getParsedIPBlocks() { return this->parsedIPBlocks; }

private:
    
    // Pointer to the environment singleton (gives output stream and some useful parameters)
    Environment *env;

    // Private fields
    list<InetAddress> parsedIPs;
    list<NetworkAddress> parsedIPBlocks;
    
    // Private methods
    void parse(string input, char separator);
    void removeDuplicata(list<InetAddress> *lIPs, 
                         list<NetworkAddress> *lIPBlocks, 
                         NetworkAddress block);
};

#endif /* TARGETPARSER_H_ */
