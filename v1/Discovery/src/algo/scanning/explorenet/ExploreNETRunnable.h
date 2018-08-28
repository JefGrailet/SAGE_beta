/*
 * ExploreNETRunnable.h
 *
 *  Created on: Oct 04, 2014
 *      Author: grailet
 *
 * This class fuses the classes ExploreNETRunnableBox and ExploreNETRunnableSingleInput from the 
 * original ExploreNET (v2.1) into a single thread class. The goal is to redesign this part of 
 * ExploreNET in order to embed it in a larger topology discovery tool (as the class 
 * ExploreNETRunnableMultipleInput is expected to be useless in this context).
 */

#ifndef EXPLORENETRUNNABLE_H_
#define EXPLORENETRUNNABLE_H_

#include "../../../common/thread/Runnable.h"
#include "../../../common/thread/Thread.h"
#include "../../../common/date/TimeVal.h"
#include "../../../common/inet/InetAddress.h"
#include "../../../common/thread/Mutex.h"
#include "../../../prober/exception/SocketException.h"
#include "../../../prober/icmp/DirectICMPProber.h"
#include "../../../prober/structure/ProbeRecord.h"
#include "../../Environment.h"
#include "../../structure/SubnetSite.h"
#include "../../structure/SubnetSiteSet.h"
#include "SubnetInferrer.h"

class ExploreNETRunnable : public Runnable
{
public:

    // Mutual exclusion object used when writing in the subnet site set
    static Mutex sssMutex;

    // Possible results of ExploreNET
    enum ExploreNETResultType
    {
        SUCCESSFULLY_INFERRED_REMOTE_SUBNET_SITE,
        SUCCESSFULLY_INFERRED_LOCAL_SUBNET_SITE, // Contains alive IP addresses of LAN and its subnet mask
        UNNECESSARY_PROBING, // For an optimization (starting from TreeNET v2.1 and onwards)
        DUMMY_LOCAL_SUBNET_SITE, // Contains only LAN subnet mask
        NULL_SUBNET_SITE,
        UNRESPONSIVE_IP_EXCEPTION,
        NO_TTL_ESTIMATION_1, // TTL estimation was aborted (too many consecutive unsuccessful probes)
        NO_TTL_ESTIMATION_2, // TTL estimation was aborted (too many redundant responsive IPs)
        NO_TTL_ESTIMATION_3, // TTL estimation was aborted (TTL became too large)
        NO_TTL_ESTIMATION_4, // TTL estimation was aborted (unknown reason)
        FOUND_EXPECTED_TTL, // (April 2018) Target had expected TTL and it was confirmed by probing
        FOUND_UNEXPECTED_TTL, // (April 2018) Target had expected TTL but distance evaluation discovered another TTL
        UNDESIGNATED_PIVOT_INTERFACE_EXCEPTION,
        SHORT_TTL_EXCEPTION
    };

    // Constructor
    ExploreNETRunnable(Environment *env, 
                       InetAddress &target, 
                       unsigned short lowerBoundSrcPortORICMPid = DirectProber::DEFAULT_LOWER_SRC_PORT_ICMP_ID,
                       unsigned short upperBoundSrcPortORICMPid = DirectProber::DEFAULT_UPPER_SRC_PORT_ICMP_ID,
                       unsigned short lowerBoundDstPortORICMPseq = DirectProber::DEFAULT_LOWER_DST_PORT_ICMP_SEQ,
                       unsigned short upperBoundDstPortICMPseq = DirectProber::DEFAULT_UPPER_DST_PORT_ICMP_SEQ);
    
    // Destructor, run method and print out method
    ~ExploreNETRunnable();
    void run();

private:
    
    // Private fields: Environment singleton, target IP and subnet inferrer object
    Environment *env;
    InetAddress target;
    SubnetInferrer sinf;
};

#endif /* EXPLORENETRUNNABLE_H_ */
