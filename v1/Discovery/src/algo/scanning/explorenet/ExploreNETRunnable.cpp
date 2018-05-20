/*
 * ExploreNETRunnable.cpp
 *
 *  Created on: Oct 04, 2014
 *      Author: grailet
 *
 * This class fuses the classes ExploreNETRunnableBox and ExploreNETRunnableSingleInput from
 * the original ExploreNET (v2.1) into a single thread class. The goal is to redesign this
 * part of ExploreNET in order to embed it in a larger topology discovery tool (as the
 * class ExploreNETRunnableMultipleInput is expected to be useless in this context).
 */

#include "ExploreNETRecord.h"
#include "ExploreNETRunnable.h"

// 7 next lines are for print out purpose only
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <iomanip>
using std::left;
using std::right;

Mutex ExploreNETRunnable::sssMutex(Mutex::ERROR_CHECKING_MUTEX);

ExploreNETRunnable::ExploreNETRunnable(Environment *e, 
                                       InetAddress &t, 
                                       unsigned short lowerBoundSrcPortORICMPid, 
                                       unsigned short upperBoundSrcPortORICMPid, 
                                       unsigned short lowerBoundDstPortORICMPseq, 
                                       unsigned short upperBoundDstPortICMPseq) throw (SocketException):
env(e), 
target(t), 
sinf(e, 
     lowerBoundSrcPortORICMPid, 
     upperBoundSrcPortORICMPid, 
     lowerBoundDstPortORICMPseq, 
     upperBoundDstPortICMPseq)
{
}

ExploreNETRunnable::~ExploreNETRunnable() {}

/**
 * Usual run method.
 */

void ExploreNETRunnable::run()
{
    InetAddress t;
    SubnetSite *site = NULL;
    unsigned short res;
    
    // Parameters obtained through env
    SubnetSiteSet *subnetSet = env->getSubnetSet();
    NetworkAddress LAN = env->getLAN();
    unsigned char startTTL = env->getStartTTL();
    bool exploreLANExplicitly = env->exploringLANExplicitly();
    bool useLowerBorderAsWell = env->usingLowerBorderAsWell();

    t = this->target;
    if(t.isUnset())
    {
        return;
    }
    
    try
    {
        // Infers local subnet if the target is within the range of the LAN
        if(LAN.subsumes(t))
        {
            // Infers local subnet with all IPs or use local information
            if(exploreLANExplicitly == true)
            {
                site = sinf.inferLocalAreaSubnet(t, LAN);
                res = ExploreNETRunnable::SUCCESSFULLY_INFERRED_LOCAL_SUBNET_SITE;
            }
            else
            {
                site = sinf.inferDummyLocalAreaSubnet(t, LAN);
                res = ExploreNETRunnable::DUMMY_LOCAL_SUBNET_SITE;
            }
        }
        // Infers remote subnet
        else
        {
            site = sinf.inferRemoteSubnet(t, false, startTTL, useLowerBorderAsWell);
            if(site != 0 && site->getInferredSubnetPrefixLength() <= 32)
            {
                if(site->getStatus() == SubnetSite::NOT_PREPARED_YET)
                {
                    res = ExploreNETRunnable::SUCCESSFULLY_INFERRED_REMOTE_SUBNET_SITE;
                }
                else
                {
                    res = ExploreNETRunnable::UNNECESSARY_PROBING;
                }
            }
            else
            {
                res = ExploreNETRunnable::NULL_SUBNET_SITE;
            }
        }
    }
    catch(SocketException &se)
    {
        Environment::consoleMessagesMutex.lock();
        ostream *out = env->getOutputStream();
        (*out) << "Probed " << t << ": critical failure at probing time." << endl;
        Environment::consoleMessagesMutex.unlock();
        
        return;
    }
    catch(NoTTLEstimationException &e)
    {
        string msg = e.what();
        std::size_t posTrueMsg = msg.find(":");
        if(posTrueMsg != std::string::npos)
        {
            string trueMsg = msg.substr(posTrueMsg + 2);
            if(trueMsg.compare("too many consecutive anonymous replies.") == 0)
                res = ExploreNETRunnable::NO_TTL_ESTIMATION_1;
            else if(trueMsg.compare("too many redundant responsive IPs.") == 0)
                res = ExploreNETRunnable::NO_TTL_ESTIMATION_2;
            else if(trueMsg.compare("estimated TTL is too large.") == 0)
                res = ExploreNETRunnable::NO_TTL_ESTIMATION_3;
            else
                res = ExploreNETRunnable::NO_TTL_ESTIMATION_4; // To make the compiler happy
            
            /*
             * Not the most elegant way to deal with this exception, but still easier and lighter 
             * than creating separate classes of exception.
             */
        }
        else
            res = ExploreNETRunnable::NO_TTL_ESTIMATION_4;
    }
    catch(ExpectedTTLException &e)
    {
        string msg = e.what();
        std::size_t posDiff = msg.find("different");
        if(posDiff != std::string::npos)
            res = ExploreNETRunnable::FOUND_UNEXPECTED_TTL;
        else
            res = ExploreNETRunnable::FOUND_EXPECTED_TTL;
    }
    catch(UnresponsiveIPException &e)
    {
        res = ExploreNETRunnable::UNRESPONSIVE_IP_EXCEPTION;
    }
    catch(UndesignatedPivotInterface &e)
    {
        res = ExploreNETRunnable::UNDESIGNATED_PIVOT_INTERFACE_EXCEPTION;
    }
    catch(ShortTTLException &e)
    {
        res = ExploreNETRunnable::SHORT_TTL_EXCEPTION;
    }
    
    string stringResult = ""; // For output purpose
    
    // Registering the site in the set
    if(res == ExploreNETRunnable::SUCCESSFULLY_INFERRED_REMOTE_SUBNET_SITE ||
       res == ExploreNETRunnable::SUCCESSFULLY_INFERRED_LOCAL_SUBNET_SITE ||
       res == ExploreNETRunnable::DUMMY_LOCAL_SUBNET_SITE)
    {
        sssMutex.lock();
        unsigned short insertionResult = subnetSet->addSite(site);
        string networkAddressStr = site->getInferredNetworkAddressString();
        
        if(insertionResult == SubnetSiteSet::SMALLER_SUBNET || 
           insertionResult == SubnetSiteSet::KNOWN_SUBNET)
        {
            delete site;
            stringResult += "inferred " + networkAddressStr;
            if(insertionResult == SubnetSiteSet::SMALLER_SUBNET)
                stringResult += ", merged with a larger or equivalent subnet";
            else
                stringResult += ", already known subnet";
        }
        else
        {
            stringResult += "inferred " + networkAddressStr + ", a new subnet";
            if(insertionResult == SubnetSiteSet::BIGGER_SUBNET)
                stringResult += " (merged with smaller, previously inferred subnets)";
            
            // Saves an ExploreNET record, but only if asked by the user.
            if(env->savingExploreNETResults())
            {
                ExploreNETRecord *rec = new ExploreNETRecord(t, 
                                                             networkAddressStr, 
                                                             site->getSubnetPositioningCost(), 
                                                             site->getSubnetInferenceCost(), 
                                                             site->getAlternativeNetworkAddressString());
                
                env->pushExploreNETRecord(rec);
            }
        }
        
        site = NULL;
        sssMutex.unlock();
    }
    // Failure: display cause of the problem in console if showInferenceFailures is true
    else
    {
        /*
         * TreeNET v2.1 and onwards (including derivates): no inference for IP encompassed by a 
         * /20 block with the same TTL for Pivot IPs, because previous inference/expansion led to 
         * subnet(s) with no Contra-Pivot to ensure their soundness.
         */
        
        if(res == ExploreNETRunnable::UNNECESSARY_PROBING)
        {
            stringResult += "no inference, as pivot TTL is the same as for IPs in ";
            stringResult += site->getInferredNetworkAddressString();
            stringResult += " (IP range to avoid)";
            
            // Avoids deleting the /20 block afterwards
            site = 0;
        }
        // Other errors
        else if(res == ExploreNETRunnable::NULL_SUBNET_SITE)
            stringResult += "failure, subnet seems to not exist";
        else if(res == ExploreNETRunnable::SHORT_TTL_EXCEPTION)
            stringResult += "no inference, subnet TTL (pivot TTL) must be at least two";
        else if(res == ExploreNETRunnable::UNDESIGNATED_PIVOT_INTERFACE_EXCEPTION)
            stringResult += "no inference, undesignated pivot IP address";
        else if(res == ExploreNETRunnable::NO_TTL_ESTIMATION_1)
            stringResult += "aborted distance (in TTL) estimation because of too many consecutive anonymous hops";
        else if(res == ExploreNETRunnable::NO_TTL_ESTIMATION_2)
            stringResult += "aborted distance (in TTL) estimation because of too many redundant responsive IPs";
        else if(res == ExploreNETRunnable::NO_TTL_ESTIMATION_3)
            stringResult += "aborted distance (in TTL) estimation because the current distance is too big";
        else if(res == ExploreNETRunnable::UNRESPONSIVE_IP_EXCEPTION)
            stringResult += "no inference, unresponsive target IP address";
        else if(res == ExploreNETRunnable::FOUND_EXPECTED_TTL)
            stringResult += "no inference, target TTL matches (contra-)pivot TTL in a previously discovered subnet";
        else if(res == ExploreNETRunnable::FOUND_UNEXPECTED_TTL)
            stringResult += "no inference, target belongs to a previously discovered subnet despite differing TTLs";
        else
            stringResult += "error in the code; uninitialized/unknown error type";
    }

    // Delete the site object if initialized
    if(site != 0)
    {
        delete site;
        site = 0;
    }
    
    Environment::consoleMessagesMutex.lock();
        
    ostream *out = env->getOutputStream();
    
    // Debug/verbose stuff
    if(env->getDisplayMode() == Environment::DISPLAY_MODE_VERBOSE)
    {
        if(res == ExploreNETRunnable::SUCCESSFULLY_INFERRED_REMOTE_SUBNET_SITE ||
           res == ExploreNETRunnable::SUCCESSFULLY_INFERRED_LOCAL_SUBNET_SITE ||
           res == ExploreNETRunnable::DUMMY_LOCAL_SUBNET_SITE)
        {
            (*out) << sinf.getExploreNETLog() << endl;
        }
    }
    else if(env->getDisplayMode() == Environment::DISPLAY_MODE_DEBUG)
    {
        (*out) << sinf.getExploreNETLog() << endl;
    }
    
    // Default message (printed in any situation)
    (*out) << "Probed " << t << ": " << stringResult << "." << endl;
    
    Environment::consoleMessagesMutex.unlock();
    
    return;
}