/*
 * IPTableEntry.h
 *
 *  Created on: Sep 30, 2015
 *      Author: jefgrailet
 *
 * This file defines a class "IPTableEntry" which extends the InetAddress class, and constitutes, 
 * as the name suggests, an entry of the IP dictionnary (IPLookUpTable). It's an extension of the 
 * InetAddress class to add fields to maintain data collected during probing, such as the timeout 
 * delay with which the first successful probe to this IP was obtained or alias resolution data.
 */

#ifndef IPTABLEENTRY_H_
#define IPTABLEENTRY_H_

#include <string>
using std::string;
#include <list>
using std::list;

#include "../../common/date/TimeVal.h"
#include "../../common/inet/InetAddress.h"

class IPTableEntry : public InetAddress
{
public:

    // Constants to represent uninitialized values
    const static unsigned char NO_KNOWN_TTL = (unsigned char) 255;
    const static unsigned long DEFAULT_TIMEOUT_SECONDS = 2;
    
    // Minimum amount of [Token, IP ID] pairs
    const static unsigned short MIN_ALIAS_RESOLUTION_PAIRS = 3;
    
    // Possible "classes" of IP ID counter
    enum IPIDCounterClasses
    {
        NO_IDEA, // No available IP ID data to infer anything, or not inferred yet
        HEALTHY_COUNTER, // Increases normally, at most one rollover observed in the sequence
        FAST_COUNTER, // Increases at a fast pace (more than one rollover in the sequence)
        RANDOM_COUNTER, // Increases abnomally fast; the sent IP-IDs are probably random
        ECHO_COUNTER // Echoes the IP-ID that was in the initial probe
    };
    
    // Constructor, destructor
    IPTableEntry(InetAddress ip, unsigned short nbIPIDs);
    ~IPTableEntry();
    
    /*
     * Resets alias resolution hints. This method is called by the constructor, but can also (and 
     * should) be called after pre-alias resolution on IPs being labels of multi-label network 
     * tree nodes. This is why this method is public.
     */
    
    void resetARHints();
    
    // Accessers/setters
    inline unsigned char getTTL() { return this->TTL; }
    inline TimeVal getPreferredTimeout() { return this->preferredTimeout; }
    inline void setTTL(unsigned char TTL) { this->TTL = TTL; }
    inline void setPreferredTimeout(TimeVal timeout) { this->preferredTimeout = timeout; }
    
    /*
     * Remark (March 2017): from now on, the TTL field will contain the shortest observed TTL for 
     * that IP. This is relevant for the IPs appearing in traceroute records, as it makes the user 
     * able to re-compute the stretch distances using the .subnet and .ip dump with a script (like 
     * a Python script). The IPTableEntry class also now offers a list of unsigned char to keep 
     * track of all hop distances at which an IP was observed in traceroute records. It is however 
     * not written in the .ip file because easily re-computable.
     */
    
    inline bool sameTTL(unsigned char TTL) { return (this->TTL != 0 && this->TTL == TTL); }
    bool hasHopCount(unsigned char hopCount);
    void recordHopCount(unsigned char hopCount);
    
    // (April 2018) Setter/accesser for expected TTL
    inline unsigned char getExpectedTTL() { return this->eTTL; }
    inline void setExpectedTTL(unsigned char eTTL) { this->eTTL = eTTL; }
    
    // Comparison method for sorting purposes
    static bool compare(IPTableEntry *ip1, IPTableEntry *ip2);
    
    // Various boolean methods related to alias resolution
    bool hasIPIDData();
    bool safeIPIDData();
    bool hasDNS();
    
    // Methods to handle "processedForAR" flag
    inline bool isProcessedForAR() { return this->processedForAR; }
    inline void raiseFlagProcessed() { this->processedForAR = true; }
    inline void resetFlagProcessed() { this->processedForAR = false; }
    
    // Accessers for alias resolution data
	inline unsigned long getProbeToken(unsigned short index) { return this->probeTokens[index]; }
	inline unsigned short getIPIdentifier(unsigned short index) { return this->IPIdentifiers[index]; }
	inline bool getEcho(unsigned short index) { return this->echoMask[index]; }
	inline unsigned long getDelay(unsigned short index) { return this->delays[index]; }
	inline string getHostName() { return this->hostName; }
	inline double getVelocityLowerBound() { return this->velocityLowerBound; }
	inline double getVelocityUpperBound() { return this->velocityUpperBound; }
	inline unsigned short getIPIDCounterType() { return this->IPIDCounterType; }
	inline unsigned char getEchoInitialTTL() { return this->echoInitialTTL; }
	inline bool repliesToTSRequest() { return this->replyingToTSRequest; }
	inline InetAddress getPortUnreachableSrcIP() { return this->portUnreachableSrcIP; }
    
    // Setters for alias resolution data
	inline void setProbeToken(unsigned short index, unsigned long pt) { this->probeTokens[index] = pt; }
	inline void setIPIdentifier(unsigned short index, unsigned short ii) { this->IPIdentifiers[index] = ii; }
	inline void setEcho(unsigned short index) { this->echoMask[index] = true; }
	inline void resetEcho(unsigned short index) { this->echoMask[index] = false; }
	inline void setDelay(unsigned short index, unsigned long d) { this->delays[index] = d; }
	inline void setHostName(string hn) { this->hostName = hn; }
	inline void setVelocityLowerBound(double vlb) { this->velocityLowerBound = vlb; }
	inline void setVelocityUpperBound(double vub) { this->velocityUpperBound = vub; }
	inline void setCounterType(unsigned short cType) { this->IPIDCounterType = cType; }
	inline void setEchoInitialTTL(unsigned char iTTL) { this->echoInitialTTL = iTTL; }
	inline void setReplyingToTSRequest() { this->replyingToTSRequest = true; }
	inline void resetReplyingToTSRequest() { this->replyingToTSRequest = false; }
	inline void setPortUnreachableSrcIP(InetAddress srcIP) { this->portUnreachableSrcIP = srcIP; }
	
	// October 2017: methods to handle route hop flags
    inline bool isARouteHop() { return this->routeHop; }
    inline bool isANeighborhoodLabel() { return this->neighborhoodLabel; }
    inline void markAsRouteHop() { this->routeHop = true; }
    inline void markAsNeighborhoodLabel() { this->neighborhoodLabel = true; }
    
    // August 2017: methods to handle pre-aliases (see fields for details).
    inline list<InetAddress> *getPreAliases() { return &preAliases; }
    bool hasPreAlias(InetAddress IP);
    void recordPreAlias(InetAddress IP);
	
	// toString() methods (for outputting an entry in a dump file, either plain or fingerprint)
    string toString();
    string toStringFingerprint();

private:

    // Basic details: TTL, timeout used for the first reply, observed hop counts
    unsigned char TTL;
    TimeVal preferredTimeout;
    list<unsigned char> hopCounts;
    
    // (April 2018) Expected TTL; used during scanning to reduce probing (see NetworkScanner.cpp)
    unsigned char eTTL;
    
    // Alias resolution hints
    unsigned short nbIPIDs;
    unsigned long *probeTokens;
	unsigned short *IPIdentifiers;
	bool *echoMask;
	unsigned long *delays;
	string hostName;
	bool replyingToTSRequest;
	InetAddress portUnreachableSrcIP;
	
	/*
	 * About echo mask: it records "true" for the corresponding index when the IP-ID is the same 
	 * as in the initial probe. The reason why this is done for every IP-ID is to avoid bad 
	 * diagnosis when a collision (= the "echo" is a pure coincidence) occurs, which might occur 
	 * during large campaigns. If every IP-ID is an echo, then it is an "echo" counter. Otherwise, 
	 * it is either a healthy, fast or random counter.
	 */
	
	// Flag to know if this IP has been processed for alias resolution
	bool processedForAR;
	
	// Data inferred from the probes which collected IP IDs
	double velocityLowerBound, velocityUpperBound;
	unsigned short IPIDCounterType;
	unsigned char echoInitialTTL; // Inferred initial TTL of an ECHO reply packet
	
	/*
     * October 2017: additionnal fields to ease graph building. They are never outputted. 
     * routeHop/neighborhoodLabel are just flags telling if the IP appears as a route hop or as 
     * (part of) a neighborhood label.
     */
    
    bool routeHop;
    bool neighborhoodLabel;
	
	/*
	 * "Pre-alias" list. A pre-alias is an alias that was discovered during graph building because 
	 * there were multiple possible IPs for a peer, motivating alias resolution to ensure these 
	 * IPs belong to distinct devices (or the contrary). Storing pre-aliases is motivated by the 
	 * fact that the alias resolution hints for the concerned IPs will be overwritten by the new 
	 * hints collected during the full alias resolution. Having pre-aliases is a way to keep track 
	 * of aliases obtained during graph building, though the full alias resolution should 
	 * theoretically recover them (unless loss of connectivity, for instance). This said, it is 
	 * possible to recover the hints for "pre-aliasing" by looking at the console output produced 
	 * at graph building.
	 *
	 * A few remarks:
	 * -pre-aliases are only obtained via reliable methods such as Ally or iffinder's approach 
	 *  (based on UDP probes).
	 * -pre-aliases also existed in later versions of TreeNET (they were created in August 2017) 
	 *  but were meant to "pre-process" multi-label nodes by applying alias resolution on the 
	 *  labels first. The full alias resolution would then be conducted on each "pre-alias" plus 
	 *  the IPs from subnets which the penultimate hop is a label. In a sense, this is kind of an 
	 *  ancestor of the alias resolution conducted during graph building.
	 */
	
	list<InetAddress> preAliases;
	
};

#endif /* IPTABLEENTRY_H_ */
