/*
 * SubnetSite.h
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * From ExploreNET v2.1, edited by J.-F. Grailet (starting from October 2014) for the needs of 
 * TreeNET and its different versions, and kept in SAGE with minor modifications. This particular 
 * version of the class is a dumbed down version of the class found in "Discovery", as there is 
 * no subnet inference (just subnet parsing) in "Fusion".
 */

#ifndef SUBNETSITE_H_
#define SUBNETSITE_H_

#include <list>
using std::list;
#include <iostream>
using std::ostream;
#include <string>
using std::string;

#include "SubnetSiteNode.h"
#include "RouteInterface.h"
#include "../../common/inet/NetworkAddress.h"

class SubnetSite
{
public:
    
    // Possible status for a subnet before/after refinement
    enum RefinementStatus
    {
        UNDEFINED_SUBNET, // Refinement failed to improve this subnet
        ACCURATE_SUBNET, // Subnet contains its contra-pivot
        ODD_SUBNET, // Subnet contains several candidates for contra-pivot, lowest one is taken
        SHADOW_SUBNET // Subnet most probably exists, but we cannot find (contra)pivot(s) for sure**
    };
    
    /*
     * **expanding this subnet collides it with accurate subnets for which the TTLs do not match.
     * For example: we expand a /32 subnet for which the only IP is at TTL 9, but expanding it to 
     * /29 makes it cover another accurate subnet for which the contrapivot is at TTL 7 and 
     * pivot(s) at TTL 8. Therefore making this subnet expand further will eventually lead to 
     * merging it with accurate subnets, resulting in one big wrong subnet. However, we can still 
     * infer that the subnet we are expanding exists. It is then labelled as a "shadow" subnet.
     */

    // Constructor/destructor
    SubnetSite();
    ~SubnetSite();

    list<SubnetSiteNode*> *getSubnetIPList() { return &IPlist; }
    void clearIPlist();
    
    // Basic setters and accessers
    inline void setInferredSubnetBaseIP(InetAddress sbi) { this->inferredSubnetBaseIP = sbi; }
    inline void setInferredSubnetPrefixLength(unsigned char spl) { this->inferredSubnetPrefix = spl; }
    
    inline InetAddress getInferredSubnetBaseIP() { return this->inferredSubnetBaseIP; }
    inline unsigned char getInferredSubnetPrefixLength() { return this->inferredSubnetPrefix; }
    
    inline int getTotalSize() { return IPlist.size(); }
    inline NetworkAddress getInferredNetworkAddress() { return NetworkAddress(this->inferredSubnetBaseIP, this->inferredSubnetPrefix); }
    
    // Accessers implemented in SubnetSite.cpp (longer implementation)
    string getInferredNetworkAddressString();
    
    // Method to insert a new interface in the IP list (made public for SubnetRefiner class)
    inline void insert(SubnetSiteNode *ssn) { this->IPlist.push_front(ssn); }
    
    /*
     * ------------------------------
     * Methods for refinement/tracing
     * ------------------------------
     * The following methods were all added during the development of TreeNET (all versions). They 
     * allow the manipulation of refinement fields and traceroute data, which are handled after 
     * the subnet inference.
     */
    
    // Gets the nodes from ss that are not listed in this subnet (returns amount; ss is emptied)
    unsigned short mergeNodesWith(SubnetSite *ss);
    
    // Method to recompute the shortest/greatest TTL after parsing
    void completeRefinedData();
    
    // Accessers to refinement data
    inline unsigned short getStatus() { return this->status; }
    inline InetAddress &getContrapivot() { return this->contrapivot; }
    inline unsigned char getShortestTTL() { return this->TTL1; }
    inline unsigned char getGreatestTTL() { return this->TTL2; }
    
    // Setters for refinement data and pivot/prefix edition
    inline void setStatus(unsigned short rs) { this->status = rs; }
    inline void setContrapivot(InetAddress &rc) { this->contrapivot = rc; }
    inline void setShortestTTL(unsigned char rst) { this->TTL1 = rst; }
    inline void setGreatestTTL(unsigned char rgt) { this->TTL2 = rgt; }
        
    // Method to obtain a pivot address of this subnet after refinement
    InetAddress getPivot();
    
    // Basic methods for route manipulation.
    inline void setRouteTarget(InetAddress rt) { this->routeTarget = rt; }
    inline void setRouteSize(unsigned short rs) { this->routeSize = rs; }
    inline void setRoute(RouteInterface *route) { this->route = route; }
    inline InetAddress getRouteTarget() { return this->routeTarget; }
    inline unsigned short getRouteSize() { return this->routeSize; }
    inline RouteInterface *getRoute() { return this->route; }
    inline bool hasValidRoute() { return (this->routeSize > 0 && this->route != NULL); }
    
    // Factors in the "penultimateShift"
    inline unsigned short getFinalRouteSize() { return this->routeSize - this->penultimateShift; }
    
    // Next methods assume the calling code previously checked there is a valid route.
    bool hasCompleteRoute(); // Returns true if the route has no missing/anonymous hop
    bool hasIncompleteRoute(); // Dual operation (true if the route has missing/anonymous hop)
    unsigned short countMissingHops(); // Returns amount of missing/anonymous hops
    
    // Comparison methods for sorting purposes
    static bool compare(SubnetSite *ss1, SubnetSite *ss2);
    static bool compareAlt(SubnetSite *ss1, SubnetSite *ss2);
    static bool compareRoutes(SubnetSite *ss1, SubnetSite *ss2);
    
    // Method checking if the current subnet encompasses another or not
    bool encompasses(SubnetSite *ss2);
    
    /*
     * -------------
     * Miscellaneous
     * -------------
     * Also added by TreeNET (all versions), these methods are essentially used to evaluate the 
     * subnet after both inference and refinement.
     */
    
    bool contains(InetAddress i); // True if i is within subnet boundaries
    bool hasPivot(InetAddress i); // Similar to contains(), but i cannot be a contra-pivot IP
    SubnetSiteNode* getNode(InetAddress li); // Returns SubnetSiteNode object associated to li, and NULL if not found
    
    /*
     * Method to output a refined subnet as a string. It is only available for such subnets, as it 
     * will output an empty string if the subnet is neither ACCURATE, ODD nor SHADOW. The output 
     * string lists on 4 lines the following content: subnet CIDR notation, refinement status, the 
     * list of responsive interfaces along their TTLs (format: [IP] - [TTL], separation with a 
     * comma) and the route to the subnet (as a list of interfaces separated with a comma). Some 
     * route steps might be tagged or replaced with "Missing"/"Anonymous" depending on the 
     * "completeness" of the route.
     */
    
    string toString();
    
    /*
     * Special method to evaluate the credibility of the subnet. Indeed, for several reasons,
     * like redirections or asymetric paths in load balancers, an originally ACCURATE or ODD 
     * subnet might can have outliers, notably after filling. To tell which subnet looks sound, 
     * this method computes the amount of live interfaces in the subnet and computes their 
     * proportion. Ideally, there should be one IP at the shortest TTL and all the others at
     * this TTL+1, but depending on the proportions, we can consider a subnet with several
     * candidates for contra-pivots is credible (e.g., there are 2 such interfaces including a 
     * back-up interface).
     */
    
    bool isCredible();
    
    // Method to obtain the capacity (i.e. number of IP addresses covered by this subnet)
    unsigned int getCapacity();
    
    /*
     * -----------------------
     * Graph preparation phase
     * -----------------------
     * Before building the graph, a small preparation phase is required. It mainly consists in 
     * computing the neighborhood label for each subnet. This label consists of the association 
     * of the last regular IP interface appearing in the route to the subnet with an amount of 
     * anomalies, a positive integer value. Initially set to 0, this amount remains untouched 
     * except if the route ends in an unusual way, such as (several consecutive) anonymous hop(s), 
     * a cycle or (an) IP(s) belonging to the subnet itself. This pair of values allow aggregation 
     * of subnets of a same label even if the route does not end with a "valid" last hop (i.e., 
     * any IP that does not fall into the "anomaly" definition).
     */
    
    // Accessers to dedicated fields
    inline InetAddress getNeighborhoodLabelIP() { return this->neighborhoodLabelIP; }
    inline unsigned short getNeighborhoodLabelAnomalies() { return this->neighborhoodLabelAnomalies; }
    
    // Neighborhood label computation (nothing if route is missing) and string representation
    void computeNeighborhoodLabel();
    string getNeighborhoodLabelString();

private:
    
    // Private fields (most of these fields already existed in ExploreNET)
    list<SubnetSiteNode*> IPlist;
    InetAddress inferredSubnetBaseIP;
    unsigned char inferredSubnetPrefix;
    
    // Fields dedicated to refinement data
    unsigned short status;
    InetAddress contrapivot;
    unsigned short TTL1, TTL2; // Shortest and greatest TTL for this subnet
    
    // Fields for traceroute data
    InetAddress routeTarget; // To keep track of the target IP used during traceroute
    unsigned short routeSize;
    RouteInterface *route;
    unsigned short penultimateShift; // March 2018: special addition
    
    /*
     * On penultimateShift: set to 0 in most cases, this field can get a positive value in very 
     * specific cases to "shift" the penultimate hop.
     * 
     * In particular, it is used to fix the issue of the "fake" last hop. If the last hop belongs 
     * to the subnet itself without being a contra-pivot IP of the same subnet, then this last hop 
     * was likely edited by the last hop router. Some routers (Cisco or Juniper) reply for IPs 
     * they provide access to and before decrementing the TTL value, hiding the interface that 
     * actually replied in the process. It therefore hides an interface in the process (which 
     * cannot be seen) and causes the last hop to be the probed IP. This problem is mitigated by 
     * removing the last hop (the before last hop should belong to the incriminated router).
     */
    
    // Neighborhood label data
    InetAddress neighborhoodLabelIP;
    unsigned short neighborhoodLabelAnomalies;
    
};

#endif /* SUBNETSITE_H_ */
