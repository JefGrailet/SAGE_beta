/*
 * Environment.h
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * Environment is a class which sole purpose is to provide access to structures or constants (for 
 * the current execution, e.g. timeout delay used for probing) which are relevant to the different 
 * parts of the program, such that each component does not have to be passed individually each 
 * time a new object is instantiated.
 *
 * Note: it was originally first implemented in TreeNET v2.0 and named "TreeNETEnvironment".
 */

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

#include <ostream>
using std::ostream;
#include <fstream>
using std::ofstream;

#include "../common/thread/Mutex.h"
#include "../common/date/TimeVal.h"
#include "../common/inet/InetAddress.h"
#include "../prober/DirectProber.h"
#include "scanning/explorenet/ExploreNETRecord.h"
#include "utils/StopException.h" // Not used directly here, but provided to all classes that need it this way
#include "structure/IPLookUpTable.h"
#include "structure/SubnetSiteSet.h"
#include "graph/Aggregate.h"

class Environment
{
public:

    // Constants to represent probing protocols
    const static unsigned short PROBING_PROTOCOL_ICMP = 1;
    const static unsigned short PROBING_PROTOCOL_UDP = 2;
    const static unsigned short PROBING_PROTOCOL_TCP = 3;

    // Constants to denote the different modes of verbosity (introduced in TreeNET v3.0)
    const static unsigned short DISPLAY_MODE_LACONIC = 0; // Default
    const static unsigned short DISPLAY_MODE_SLIGHTLY_VERBOSE = 1;
    const static unsigned short DISPLAY_MODE_VERBOSE = 2;
    const static unsigned short DISPLAY_MODE_DEBUG = 3;

    // Mutex objects used when printing out additionnal messages (slightly verbose to debug) and triggering emergency stop
    static Mutex consoleMessagesMutex;
    static Mutex emergencyStopMutex;

    // Constructor/destructor (default values for most parameters, see implementation in .cpp file)
    Environment(ostream *consoleOut, 
                bool externalLogs, 
                unsigned short probingProtocol, 
                bool saveExploreNETRecords, 
                InetAddress &localIPAddress, 
                NetworkAddress &LAN, 
                unsigned short displayMode);
    ~Environment();
    
    /************
    * Accessers *
    ************/
    
    /*
     * N.B.: the methods returning (const) references have no other point than being compatible 
     * with the libraries prober/ and common/, which were inherited from ExploreNET v2.1. 
     * Otherwise, most fields are values and can be returned as such.
     */
    
    // Data structures
    inline IPLookUpTable *getIPTable() { return this->IPTable; }
    inline SubnetSiteSet *getSubnetSet() { return this->subnetSet; }
    inline SubnetSiteSet *getIPBlocksToAvoid() { return this->IPBlocksToAvoid; }
    
    // Probing parameters (to be set by command line options)
    inline unsigned short getProbingProtocol() { return this->probingProtocol; }
    inline const InetAddress &getLocalIPAddress() { return this->localIPAddress; }
    inline const NetworkAddress &getLAN() { return this->LAN; }
    
    // Probing parameters (to be set by configuration file)
    inline const TimeVal &getTimeoutPeriod() { return this->timeoutPeriod; }
    inline const TimeVal &getProbeRegulatingPeriod() { return this->probeRegulatingPeriod; }
    inline bool exploringLANExplicitly() { return this->exploreLANExplicitly; }
    inline bool usingLowerBorderAsWell() { return this->useLowerBorderAsWell; }
    inline bool usingDoubleProbe() { return this->doubleProbe; }
    inline bool usingFixedFlowID() { return this->useFixedFlowID; }
    inline string &getAttentionMessage() { return this->probeAttentionMessage; }
    
    // Concurrency parameters
    inline unsigned short getMaxThreads() { return this->maxThreads; }
    inline const TimeVal &getProbeThreadDelay() { return this->probeThreadDelay; }
    
    // Display parameters
    inline unsigned short getDisplayMode() { return this->displayMode; }
    inline bool debugMode() { return (this->displayMode == DISPLAY_MODE_DEBUG); }
    
    // Parameters related to algorithms themselves
    inline unsigned char getStartTTL() { return this->startTTL; }
    inline bool expandingAtPrescanning() { return this->prescanExpand; }
    inline bool usingPrescanningThirdOpinion() { return this->prescanThirdOpinion; }
    
    inline unsigned short getARNbIPIDs() { return this->ARNbIPIDs; }
    inline unsigned short getARAllyMaxDiff() { return this->ARAllyMaxDiff; }
    inline unsigned short getARAllyMaxConsecutiveDiff() { return this->ARAllyMaxConsecutiveDiff; }
    inline unsigned short getARVelocityMaxRollovers() { return this->ARVelocityMaxRollovers; }
    inline double getARVelocityBaseTolerance() { return this->ARVelocityBaseTolerance; }
    inline double getARVelocityMaxError() { return this->ARVelocityMaxError; }
    
    // Output stream and external log feature
    ostream *getOutputStream();
    inline bool usingExternalLogs() { return this->externalLogs; }
    
    /**********
    * Setters *
    **********/
    
    /*
     * N.B.: while accessers are listed by "categories", setters are listed by type to follow the 
     * same order of appearance as in the constants of Environment as well as in the class 
     * ConfigFileParser.
     */
    
    // Time parameters
    inline void setTimeoutPeriod(TimeVal timeout) { this->timeoutPeriod = timeout; }
    inline void setProbeRegulatingPeriod(TimeVal period) { this->probeRegulatingPeriod = period; }
    inline void setProbeThreadDelay(TimeVal delay) { this->probeThreadDelay = delay; }
    
    // Boolean parameters
    inline void setExploringLANExplicitly(bool val) { this->exploreLANExplicitly = val; }
    inline void setUsingLowerBorderAsWell(bool val) { this->useLowerBorderAsWell = val; }
    inline void setUsingDoubleProbe(bool val) { this->doubleProbe = val; }
    inline void setUsingFixedFlowID(bool val) { this->useFixedFlowID = val; }
    inline void setPrescanThirdOpinion(bool val) { this->prescanThirdOpinion = val; }
    inline void setPrescanExpansion(bool val) { this->prescanExpand = val; }
    
    // String parameter
    inline void setAttentionMessage(string msg) { this->probeAttentionMessage = msg; }
    
    // Integer parameters
    inline void setMaxThreads(unsigned short max) { this->maxThreads = max; }
    inline void setStartTTL(unsigned char TTL) { this->startTTL = TTL; }
    inline void setARNbIPIDs(unsigned short nb) { this->ARNbIPIDs = nb; }
    inline void setARAllyMaxDiff(unsigned short diff) { this->ARAllyMaxDiff = diff; }
    inline void setARAllyMaxConsecutiveDiff(unsigned short diff) { this->ARAllyMaxConsecutiveDiff = diff; }
    inline void setARVelocityMaxRollovers(unsigned short max) { this->ARVelocityMaxRollovers = max; }
    
    // Double parameters
    inline void setARVelocityBaseTolerance(double base) { this->ARVelocityBaseTolerance = base; }
    inline void setARVelocityMaxError(double max) { this->ARVelocityMaxError = max; }
    
    /****************
    * Other methods *
    ****************/
    
    // Methods to handle the ExploreNET records (optional feature, added Aug 29, 2016).
    inline bool savingExploreNETResults() { return this->saveExploreNETRecords; }
    inline void pushExploreNETRecord(ExploreNETRecord *r) { this->xnetRecords.push_back(r); }
    void outputExploreNETRecords(string filename);
    
    // Methods to handle the initial target IPs/ranges
    inline list<InetAddress> *getInitialTargetIPs() { return &itIPs; }
    inline list<NetworkAddress> *getInitialTargetRanges() { return &itRanges; }
    bool initialTargetsEncompass(InetAddress IP);
    unsigned int getTotalIPsInitialTargets();
    
    // Methods to handle total amounts of (successful) probes
    void updateProbeAmounts(DirectProber *proberObject);
    void resetProbeAmounts();
    inline unsigned int getTotalProbes() { return this->totalProbes; }
    inline unsigned int getTotalSuccessfulProbes() { return this->totalSuccessfulProbes; }
    
    // Method to handle the output stream writing in an output file.
    void openLogStream(string filename, bool message = true);
    void closeLogStream();
    
    /*
     * Method to trigger the (emergency) stop. It is a special method of Environment which is 
     * meant to force the program to quit when it cannot fully benefit from the resources of the 
     * host computer to conduct measurements/probing. It has a return result (though not used 
     * yet), a boolean one, which is true if the current context successfully triggered the stop 
     * procedure. It will return false it was already triggered by another thread.
     */
    
    bool triggerStop();
    
    // Method to check if the flag for emergency stop is raised.
    inline bool isStopping() { return this->flagEmergencyStop; }
    
    /*
     * Method to fill the IP dictionnary with IPs found in the routes to the subnets (when they 
     * are missing) at the end of the traceroute phase.
     */
    
    void recordRouteHopsInDictionnary();
    
    /*
     * Method to prepare the subnets and their routes for the graph building phase.
     */
    
    void prepareForGraphBuilding();

private:

    // Structures
    IPLookUpTable *IPTable;
    SubnetSiteSet *subnetSet;
    SubnetSiteSet *IPBlocksToAvoid; // Lists UNDEFINED /20 subnets where expansion should not be done
    
    /*
     * Output streams (main console output and file stream for the external logs). Having both is 
     * useful because the console output stream will still be used to advertise the creation of a 
     * new log file and emergency stop if the user requested probing details to be written in 
     * external logs.
     */
    
    ostream *consoleOut;
    ofstream logStream;
    bool externalLogs;
    bool isExternalLogOpened;
    
    // Probing parameters
    unsigned short probingProtocol;
    InetAddress &localIPAddress;
    NetworkAddress &LAN; // Avoids editing the class for = operator or default constructor (lazy)
    
    TimeVal timeoutPeriod;
    TimeVal probeRegulatingPeriod;
    bool exploreLANExplicitly;
    bool useLowerBorderAsWell;
    bool doubleProbe;
    bool useFixedFlowID;
    string probeAttentionMessage;
    
    // Algorithmic parameters
    bool prescanExpand;
    bool prescanThirdOpinion;
    unsigned char startTTL; // For subnet inference
    bool saveExploreNETRecords; // To save subnets as inferred by ExploreNET in a separate file
    unsigned short ARNbIPIDs;
    unsigned short ARAllyMaxDiff; // Max difference between two IDs for Ally application
    unsigned short ARAllyMaxConsecutiveDiff; // Max difference between two IDs for ID re-ordering
    unsigned short ARVelocityMaxRollovers;
    double ARVelocityBaseTolerance;
    double ARVelocityMaxError;
    
    // Display parameter (max. value = 3, amounts to debug mode)
    unsigned short displayMode;
    
    // Concurrency parameters
    unsigned short maxThreads;
    TimeVal probeThreadDelay;
    
    // Lists used to maintain the initial target (it) IPs/IP ranges
    list<InetAddress> itIPs;
    list<NetworkAddress> itRanges;
    
    // List for registering the ExploreNET records (optional feature)
    list<ExploreNETRecord*> xnetRecords;
    
    // Fields to record the amount of (successful) probes used during some stage (can be reset)
    unsigned int totalProbes;
    unsigned int totalSuccessfulProbes;
    
    // Flag for emergency exit
    bool flagEmergencyStop;

};

#endif /* ENVIRONMENT_H_ */
