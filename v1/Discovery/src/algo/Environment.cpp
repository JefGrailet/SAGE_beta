/*
 * Environment.cpp
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * This file implements the class defined in Environment.h. See this file for more details on the 
 * goals of such class.
 */

#include <math.h> // For pow() function
#include <sys/stat.h> // For CHMOD edition
#include <iomanip> // For header lines in the output file displaying ExploreNET records
using std::left;
using std::setw;

#include "Environment.h"

Mutex Environment::consoleMessagesMutex(Mutex::ERROR_CHECKING_MUTEX);
Mutex Environment::emergencyStopMutex(Mutex::ERROR_CHECKING_MUTEX);

Environment::Environment(ostream *consoleOut, 
                         bool externalLogs, 
                         unsigned short probingProtocol, 
                         bool saveExploreNETRecords, 
                         InetAddress &localIP, 
                         NetworkAddress &lan, 
                         unsigned short displayMode): localIPAddress(localIP), LAN(lan)
{
    this->consoleOut = consoleOut;
    this->externalLogs = externalLogs;
    this->isExternalLogOpened = false;
    this->probingProtocol = probingProtocol;
    
    // Default values for probing parameters
    this->timeoutPeriod = TimeVal(2, TimeVal::HALF_A_SECOND);
    this->probeRegulatingPeriod = TimeVal(0, 50000);
    this->exploreLANExplicitly = false; 
    this->useLowerBorderAsWell = true;
    this->doubleProbe = false;
    this->useFixedFlowID = true;
    this->probeAttentionMessage = "NOT AN ATTACK (e-mail: Jean-Francois.Grailet@uliege.be)";
    
    // Default values for algorithmic parameters
    this->prescanExpand = false;
    this->prescanThirdOpinion = false;
    this->startTTL = (unsigned char) 1;
    this->saveExploreNETRecords = saveExploreNETRecords;
    this->ARNbIPIDs = 4;
    this->ARAllyMaxDiff = 13107; // 1/5 * 2^16
    this->ARAllyMaxConsecutiveDiff = 100;
    this->ARVelocityMaxRollovers = 10;
    this->ARVelocityBaseTolerance = 0.2;
    this->ARVelocityMaxError = 0.35;
    
    this->displayMode = displayMode;
    
    // Default values for concurrency parameters
    this->maxThreads = 256;
    this->probeThreadDelay = TimeVal(0, 250000);
    
    this->totalProbes = 0;
    this->totalSuccessfulProbes = 0;
    this->flagEmergencyStop = false;
    
    this->IPTable = new IPLookUpTable(ARNbIPIDs);
    this->subnetSet = new SubnetSiteSet();
    this->IPBlocksToAvoid = new SubnetSiteSet();
}

Environment::~Environment()
{
    delete IPTable;
    delete subnetSet;
    delete IPBlocksToAvoid;
    
    for(list<ExploreNETRecord*>::iterator it = xnetRecords.begin(); it != xnetRecords.end(); it++)
    {
        delete (*it);
    }
}

ostream* Environment::getOutputStream()
{
    if(this->externalLogs || this->isExternalLogOpened)
        return &this->logStream;
    return this->consoleOut;
}

void Environment::outputExploreNETRecords(string filename)
{
    xnetRecords.sort(ExploreNETRecord::compare);

    ofstream newFile;
    newFile.open(filename.c_str());
    
    // Header lines
    newFile << left << setw(20) << "Target IP";
    newFile << left << setw(25) << "Inferred subnet";
    newFile << left << setw(6) << "SPC";
    newFile << left << setw(6) << "SIC";
    newFile << left << setw(18) << "Alternative";
    newFile << "\n";
    
    newFile << left << setw(20) << "---------";
    newFile << left << setw(25) << "---------------";
    newFile << left << setw(6) << "---";
    newFile << left << setw(6) << "---";
    newFile << left << setw(18) << "-----------";
    newFile << "\n";
    
    for(list<ExploreNETRecord*>::iterator it = xnetRecords.begin(); it != xnetRecords.end(); it++)
    {
        newFile << (*it)->toString() << "\n";
    }
    
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

bool Environment::initialTargetsEncompass(InetAddress IP)
{
    for(list<InetAddress>::iterator it = itIPs.begin(); it != itIPs.end(); ++it)
    {
        if(IP == (*it))
            return true;
    }
    
    for(list<NetworkAddress>::iterator it = itRanges.begin(); it != itRanges.end(); ++it)
    {
        NetworkAddress curRange = (*it);
        if(IP >= curRange.getLowerBorderAddress() && IP <= curRange.getUpperBorderAddress())
            return true;
    }
    
    return false;
}

unsigned int Environment::getTotalIPsInitialTargets()
{
    unsigned int totalIPs = itIPs.size();
    for(list<NetworkAddress>::iterator it = itRanges.begin(); it != itRanges.end(); ++it)
    {
        double power = 32 - (double) ((unsigned short) (*it).getPrefixLength());
        totalIPs += (unsigned int) pow(2, power);
    }
    return totalIPs;
}

void Environment::updateProbeAmounts(DirectProber *proberObject)
{
    totalProbes += proberObject->getNbProbes();
    totalSuccessfulProbes += proberObject->getNbSuccessfulProbes();
}

void Environment::resetProbeAmounts()
{
    totalProbes = 0;
    totalSuccessfulProbes = 0;
}

void Environment::openLogStream(string filename, bool message)
{
    this->logStream.open(filename.c_str());
    this->isExternalLogOpened = true;
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
    
    if(message)
        (*consoleOut) << "Log of current phase is being written in " << filename << ".\n" << endl;
}

void Environment::closeLogStream()
{
    this->logStream.close();
    this->logStream.clear();
    this->isExternalLogOpened = false;
}

bool Environment::triggerStop()
{
    /*
     * In case of loss of connectivity, it is possible several threads calls this method. To avoid 
     * multiple contexts launching the emergency stop, there is the following condition (in 
     * addition to a Mutex object).
     */
    
    if(flagEmergencyStop)
    {
        return false;
    }

    flagEmergencyStop = true;
    
    consoleMessagesMutex.lock();
    (*consoleOut) << "\nWARNING: emergency stop has been triggered because of connectivity ";
    (*consoleOut) << "issues.\nSAGE will wait for all remaining threads to complete before ";
    (*consoleOut) << "exiting without carrying out remaining probing tasks.\n" << endl;
    consoleMessagesMutex.unlock();
    
    return true;
}

void Environment::recordRouteHopsInDictionnary()
{
    list<SubnetSite*> *ssList = subnetSet->getSubnetSiteList();
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        SubnetSite *cur = (*it);
        if(!cur->hasValidRoute())
            continue;
        
        unsigned short routeSize = cur->getRouteSize();
        RouteInterface *route = cur->getRoute();
        for(unsigned short i = 0; i < routeSize; ++i)
        {
            unsigned char TTL = (unsigned char) i + 1;
            InetAddress curIP = route[i].ip;
            if(curIP == InetAddress(0))
                continue;
            
            IPTableEntry *entry = IPTable->lookUp(curIP);
            if(entry == NULL)
            {
                entry = IPTable->create(curIP);
                entry->setTTL(TTL);
            }
            else if(!entry->sameTTL(TTL) && !entry->hasHopCount(TTL))
            {
                entry->recordHopCount(TTL);
            }
            
            if(i == routeSize - 1 && !cur->hasPivot(curIP))
                entry->markAsNeighborhoodLabel();
            else
                entry->markAsRouteHop();
        }
    }
}

void Environment::prepareForGraphBuilding()
{
    list<SubnetSite*> *ssList = subnetSet->getSubnetSiteList();
    for(list<SubnetSite*>::iterator it = ssList->begin(); it != ssList->end(); ++it)
    {
        SubnetSite *cur = (*it);
        if(!cur->hasValidRoute())
            continue;
        
        cur->computeNeighborhoodLabel();
        
        unsigned short routeSize = cur->getRouteSize();
        RouteInterface *route = cur->getRoute();
        for(unsigned short i = 0; i < routeSize - 1; ++i)
        {
            InetAddress curIP = route[i].ip;
            if(curIP == InetAddress(0))
                continue;
            
            IPTableEntry *entry = IPTable->lookUp(curIP);
            if(entry == NULL)
                continue;
            
            if(entry->isANeighborhoodLabel())
                route[i].neighborhoodLabel = true;
        }
    }
}
