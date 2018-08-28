/*
 * ExpectedTTLException.cpp
 *
 *  Created on: Apr 6, 2018
 *      Author: grailet
 */

#include "ExpectedTTLException.h"

ExpectedTTLException::ExpectedTTLException(const InetAddress &dst, const string &msg):
NTmapException(msg),
destinationIP(dst)
{

}

ExpectedTTLException::~ExpectedTTLException() throw() {}
