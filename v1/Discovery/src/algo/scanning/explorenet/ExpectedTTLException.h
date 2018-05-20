/*
 * ExpectedTTLException.h
 *
 *  Created on: Apr 6, 2018
 *      Author: grailet
 */

#ifndef EXPECTEDTTLEXCEPTION_H_
#define EXPECTEDTTLEXCEPTION_H_

#include "../../../common/exception/NTmapException.h"
#include "../../../common/inet/InetAddress.h"
#include <string>
using std::string;

class ExpectedTTLException: public NTmapException
{
public:
    ExpectedTTLException(const InetAddress &destinationIP, const string &msg = "Discovered the expected TTL value for this IP.");
    virtual ~ExpectedTTLException() throw();
    InetAddress & getDestinationIP() { return destinationIP; }
    void setDestinationIP(InetAddress &destinationIP) { this->destinationIP = destinationIP; }
private:
    InetAddress destinationIP;
};

#endif /* EXPECTEDTTLEXCEPTION_H_ */
