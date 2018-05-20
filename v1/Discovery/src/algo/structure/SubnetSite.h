/*
 * SubnetSite.h
 *
 *  Created on: Jul 19, 2012
 *      Author: engin
 *
 * This class models a subnet. As the initial author and creation date suggests, it originates 
 * from ExploreNET v2.1, but it has been considerably extended over time during the development of 
 * TreeNET, a tool mapping subnets with a tree-like data structure able to go as far as alias 
 * resolution and which embeds ExploreNET for subnet inference. Most of the additions consists in 
 * refinement and traceroute data which are computed or collected after the subnet inference, such 
 * that a subnet object contains all the data needed for subsequent analysis and mapping.
 * 
 * As such, a SubnetSite object is completed in several "stages" (inference, refinement, 
 * traceroute). From an object-oriented perspective, one could declare child classes of the 
 * original SubnetSite (one per additionnal "stage") to provide the additionnal fields/methods, 
 * but it was easier at first to just extend the original class. To compensate, this class is more 
 * documented and structured than others. A future refactoring could however split this class into 
 * a kind of hierarchy.
 *
 * Modifications brought over time by J.-F. Grailet:
 * -From October 2014 to roughly end of 2015: improved coding style, new fields and methods 
 *  suited for TreeNET.
 * -Late Augustus 2016: removed the override of << operator, because it is no longer useful even 
 *  in the new debug/verbose utilities.
 * -Sept 8, 2016: slight refactoring of the class to benefit from the addition of the class 
 *  RouteInterface. There is now a single route array, and no longer two (one for the IPs along 
 *  the route, and a second for the repairment mask).
 * -Mar 27, 2017: addition of a second route for post-processing. It replaces the observed route 
 *  when available.
 * -Sept 22, 2017: removed the post-processed route.
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

class SubnetInferrer;

class SubnetSite
{
public:

    friend class SubnetInferrer; // Allows it to use private methods during inference
    
    // Possible status for a subnet before/after refinement
    enum RefinementStatus
    {
        NOT_PREPARED_YET, // Subnet exists but is not ready to be used
        ACCURATE_SUBNET, // Subnet contains its contra-pivot
        INCOMPLETE_SUBNET, // Subnet contains only one IP or all IPs are at the same hop distance
        ODD_SUBNET, // Subnet contains several candidates for contra-pivot, lowest one is taken
        SHADOW_SUBNET, // Subnet most probably exists, but we cannot find (contra)pivot(s) for sure**
        UNDEFINED_SUBNET // Refinement failed to improve this subnet
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
    
    /*
     * ---------------------
     * Methods for inference
     * ---------------------
     * The next methods are (for most of them) straight from ExploreNET v2.1 and are used to 
     * handle the main fields involved in subnet inference, such as the "cost" fields or the 
     * target, pivot, previous (TTL - 1) interfaces. The main addition brough by TreeNET here is 
     * the MA (Merge Amount) value.
     */
    
    // Tells if ExploreNET found an alternative prefix for this subnet
    bool hasAlternativeSubnet();

    // Accessers (straightforward implementation)
    inline unsigned int getSubnetPositioningCost() { return this->spCost; }
    inline unsigned int getSubnetInferenceCost() { return this->siCost; }
    inline unsigned int getMergeAmount() { return this->mergeAmount; }
    inline InetAddress &getTargetAddress() { return this->targetIPaddress; }
    inline InetAddress &getPivotAddress() { return this->pivotIPaddress; } // ! Pivot during ExploreNET inference
    inline InetAddress &getIngressInterfaceAddress() { return this->prevSiteIPaddress; }
    inline int getSubnetHopDistance() { return this->prevSiteIPaddressDistance; }
    inline unsigned char getInferredSubnetPrefixLength() { return this->inferredSubnetPrefix; }
    inline unsigned char getAlternativeSubnetPrefixLength() { return this->alternativeSubnetPrefix; }
    inline int getTotalSize() { return IPlist.size(); }
    
    inline NetworkAddress getInferredNetworkAddress() { return NetworkAddress(this->pivotIPaddress, this->inferredSubnetPrefix); }
    inline NetworkAddress getAlternativeNetworkAddress() { return NetworkAddress(this->pivotIPaddress, this->alternativeSubnetPrefix); }
    
    // Accessers implemented in SubnetSite.cpp (longer implementation)
    InetAddress getInferredSubnetContraPivotAddress(); // Returns alias if subnet inference found one
    InetAddress getAlternativeSubnetContraPivotAddress();
    int getInferredSubnetSize();
    int getAlternativeSubnetSize();
    string getInferredNetworkAddressString();
    string getAlternativeNetworkAddressString();
    
    // Handler of the merge amount value
    inline void incMergeAmount() { this->mergeAmount++; }
    inline void setMergeAmount(unsigned int ma) { this->mergeAmount = ma; }
    
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
    
    // Method to call after inference is completed to prepare refinement steps
    void prepareForRefinement();
    
    // Accessers to refinement data
    inline unsigned short getStatus() { return this->refinementStatus; }
    inline InetAddress &getContrapivot() { return this->refinementContrapivot; }
    inline unsigned char getShortestTTL() { return this->refinementTTL1; }
    inline unsigned char getGreatestTTL() { return this->refinementTTL2; }
    
    // Setters for refinement data and pivot/prefix edition
    inline void setStatus(unsigned short rs) { this->refinementStatus = rs; }
    inline void setContrapivot(InetAddress &rc) { this->refinementContrapivot = rc; }
    inline void setShortestTTL(unsigned char rst) { this->refinementTTL1 = rst; }
    inline void setGreatestTTL(unsigned char rgt) { this->refinementTTL2 = rgt; }
    inline void setPivotAddress(InetAddress &pivot) { this->pivotIPaddress = pivot; }
    inline void setInferredSubnetPrefixLength(unsigned char ispl) { this->inferredSubnetPrefix = ispl; }

    // Method to recompute the refinement status at certain points
    void recomputeRefinementStatus();
    
    // Methods to handle the special "newSubnet" flag (used during subnet inference, Nov. 2017)
    inline bool isANewSubnet() { return newSubnet; }
    inline void markAsNewSubnet() { newSubnet = true; }
    inline void markAsKnownSubnet() { newSubnet = false; }
    
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
     * Method to test if the subnet is an "artifact", i.e., a subnet which was inferred as a /32 
     * and still is after refining. While there are rare occurrences of actual /32 "in the wild", 
     * their presence in subsequent algorithmic steps can be a problem for both interpretation of 
     * the data and behavior of the program.
     */
    
    bool isAnArtifact();
    
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

    /*
     * Private methods. With the exception of prepareTTLsForRefinement(), they all originate from 
     * the original ExploreNET (v2.1 at the time it was used to implement the first TreeNET) and 
     * are only used during the subnet inference. The fact that SubnetInferrer is a friend class 
     * allows it to call them directly on a subnet (see SubnetInferrer.cpp).
     */
    
    int getSize(int filterin); // filterin determines what type of site nodes (i.e., node status) should be included in search
    unsigned char getMinimumPrefixLength(int filterin);
    void markSubnetOvergrowthElements(unsigned char barrierPrefix, int filterin);
    void markSubnetBoundaryIncompatibileElements(unsigned char basePrefix, int filterin);
    bool contains(InetAddress ip, int filterin);
    void adjustLocalAreaSubnet(NetworkAddress &localAreaNetwork); // Added later to make sure LAN prefix is set forcefully
    void adjustRemoteSubnet2(bool useLowerBorder);
    void adjustRemoteSubnet();
    void prepareTTLsForRefinement();
    
    // Private fields (most of these fields already existed in ExploreNET)
    list<SubnetSiteNode*> IPlist;
    unsigned int spCost; // Subnet positioning cost (SPC)
    unsigned int siCost; // Subnet inference cost (SIC)
    unsigned int mergeAmount; // Amount of merge operations to improve this subnet (MA)

    InetAddress targetIPaddress;
    InetAddress pivotIPaddress; // SiteRecord.destination
    InetAddress prevSiteIPaddress; // Ingress interface
    unsigned char prevSiteIPaddressDistance;

    unsigned char inferredSubnetPrefix;
    unsigned char alternativeSubnetPrefix;
    SubnetSiteNode *contraPivotNode;
    
    // Fields dedicated to refinement data
    unsigned short refinementStatus;
    InetAddress refinementContrapivot;
    unsigned short refinementTTL1, refinementTTL2; // Shortest and greatest TTL for this subnet
    
    // Special flag used to mark subnets discovered during a round of network scanning (Nov. 2017)
    bool newSubnet;
    
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
