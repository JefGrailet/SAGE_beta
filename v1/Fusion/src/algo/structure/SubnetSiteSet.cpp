/*
 * SubnetSiteSet.cpp
 *
 *  Created on: Oct 9, 2014
 *      Author: jefgrailet
 *
 * Implements the class defined in SubnetSiteSet.h (see this file to learn further about the 
 * goals of such a class).
 */

#include <fstream>
#include <sys/stat.h> // For CHMOD edition
#include "SubnetSiteSet.h"
#include "../../common/inet/NetworkAddress.h"

using namespace std;

SubnetSiteSet::SubnetSiteSet()
{
}

SubnetSiteSet::~SubnetSiteSet()
{
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        delete (*i);
    }
    siteList.clear();
}

unsigned short SubnetSiteSet::addSite(SubnetSite *ss)
{
    InetAddress lowerBorder1, upperBorder1;
    unsigned char prefixLength = ss->getInferredSubnetPrefixLength();
    if(prefixLength <= 31)
    {
        NetworkAddress na1 = ss->getInferredNetworkAddress();
        lowerBorder1 = na1.getLowerBorderAddress();
        upperBorder1 = na1.getUpperBorderAddress();
    }
    // /32 subnets
    else
    {
        lowerBorder1 = ss->getInferredSubnetBaseIP();
        upperBorder1 = lowerBorder1;
    }
    
    unsigned short toReturn = SubnetSiteSet::NEW_SUBNET; // Default return value
    
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss2 = (*i);
        InetAddress lowerBorder2, upperBorder2;
        unsigned char prefixLength2 = ss2->getInferredSubnetPrefixLength();
        
        if(prefixLength2 <= 31)
        {
            NetworkAddress na2 = ss2->getInferredNetworkAddress();
            lowerBorder2 = na2.getLowerBorderAddress();
            upperBorder2 = na2.getUpperBorderAddress();
        }
        else
        {
            lowerBorder2 = ss2->getInferredSubnetBaseIP();
            upperBorder2 = lowerBorder2;
        }
        
        if(lowerBorder2 <= lowerBorder1)
        {
            // ss2 contains ss or is equivalent
            if(upperBorder2 >= upperBorder1)
            {
                // Special (but rare) case: identical /32 subnets
                if(prefixLength == 32 && prefixLength2 == 32)
                    return SubnetSiteSet::KNOWN_SUBNET;
                
                unsigned short newInterfaces = ss2->mergeNodesWith(ss);
                if(newInterfaces > 0)
                {
                    return SubnetSiteSet::SMALLER_SUBNET;
                }
                return SubnetSiteSet::KNOWN_SUBNET;
            }
            // upperBorder2 < upperBorder1 but lowerBorder1 == lowerBorder2: ss contains ss2
            else if(lowerBorder2 == lowerBorder1)
            {
                ss->mergeNodesWith(ss2);
                siteList.erase(i--);
                delete ss2;
                
                // We do not stop here, in case there was another subnet contained in ss.
                toReturn = SubnetSiteSet::BIGGER_SUBNET;
            }
            // upperBorder2 < upperBorder1: ss2 does not contain ss
            else
            {
                continue;
            }
        }
        // lowerBorder2 > lowerBorder1
        else
        {
            // ss contains ss2
            if(upperBorder1 >= upperBorder2)
            {
                ss->mergeNodesWith(ss2);
                siteList.erase(i--);
                delete ss2;
                
                // We do not stop here, in case there was another subnet contained in ss.
                toReturn = SubnetSiteSet::BIGGER_SUBNET;
            }
            // upperBorder1 < upperBorder2: ss does not contain ss2
            else
            {
                continue;
            }
        }
    }
    
    // Inserts ss and sorts the set
    siteList.push_back(ss);
    siteList.sort(SubnetSite::compare);
    return toReturn;
}

void SubnetSiteSet::addSiteNoMerging(SubnetSite *ss)
{
    siteList.push_back(ss);
}

void SubnetSiteSet::sortSet()
{
    siteList.sort(SubnetSite::compare);
}

bool SubnetSiteSet::isCompatible(InetAddress lowerBorder, 
                                 InetAddress upperBorder, 
                                 unsigned char TTL,
                                 bool beforeAndAfter,
                                 bool shadowExpansion)
{
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss2 = (*i);
        InetAddress lowerBorder2, upperBorder2;
        unsigned char prefixLength2 = ss2->getInferredSubnetPrefixLength();
        
        if(prefixLength2 <= 31)
        {
            NetworkAddress na2 = ss2->getInferredNetworkAddress();
            lowerBorder2 = na2.getLowerBorderAddress();
            upperBorder2 = na2.getUpperBorderAddress();
        }
        // /32 subnets
        else
        {
            lowerBorder2 = ss2->getInferredSubnetBaseIP();
            upperBorder2 = ss2->getInferredSubnetBaseIP();
        }
        
        if(lowerBorder2 <= lowerBorder)
        {
            // ss2 contains hypothetical subnet or is equivalent
            if(upperBorder2 >= upperBorder)
            {
                /*
                 * In the context where isCompatible() is being used (i.e. expansion of a subnet,
                 * isCompatible() is used to prevent over-expanding), this case should never
                 * occur.
                 */
                 
                continue;
            }
            // upperBorder2 < upperBorder but lowerBorder == lowerBorder2: hypothetical subnet contains ss2
            else if(lowerBorder2 == lowerBorder)
            {
                unsigned short ss2s = ss2->getStatus();
                unsigned char ss2TTL = ss2->getGreatestTTL();
            
                /*
                 * If a shadow subnet is being expanded and encompasses an ACCURATE/ODD subnet, 
                 * it cannot be compatible with the current borders.
                 */
                
                if(shadowExpansion && (ss2s == SubnetSite::ACCURATE_SUBNET || ss2s == SubnetSite::ODD_SUBNET))
                {
                    return false;
                }
            
                /*
                 * Conditions for TTL compatibility:
                 * -If ss2 is ACCURATE/ODD, TTL must be equal to greatest TTL.
                 * -Otherwise, TTL should be equal to the greatest TTL of ss2 or the same TTL-1.
                 * -If beforeAndAfter is set to true, we also compare to the same TTL+1. Indeed, 
                 *  it can occur that we have 2 subnets each with a single responsive interface, 
                 *  with a difference of 1 TTL. The compatibility will work if we expand the 
                 *  subnet with the subnet having the shortest TTL, but with the conditions 
                 *  above, the contrary will fail, hence the additional comparison.
                 */
                
                if((ss2s == SubnetSite::ACCURATE_SUBNET || ss2s == SubnetSite::ODD_SUBNET) && TTL == ss2TTL)
                {
                    continue;
                }
                else
                {
                    unsigned char TTLMinus1 = ss2TTL - 1;
                    unsigned char TTLPlus1 = ss2TTL + 1;
                
                    if (TTL == ss2TTL || TTL == TTLMinus1 || (beforeAndAfter && TTL == TTLPlus1))
                        continue;
                    else
                        return false;
                }
            }
            // upperBorder2 < upperBorder: ss2 does not contain hypothetical subnet
            else
            {
                continue;
            }
        }
        // lowerBorder2 > lowerBorder
        else
        {
            // ss contains ss2
            if(upperBorder >= upperBorder2)
            {
                // Same verifications as above
                unsigned short ss2s = ss2->getStatus();
                unsigned char ss2TTL = ss2->getGreatestTTL();
                
                if(shadowExpansion && (ss2s == SubnetSite::ACCURATE_SUBNET || ss2s == SubnetSite::ODD_SUBNET))
                {
                    return false;
                }
                
                if((ss2s == SubnetSite::ACCURATE_SUBNET || ss2s == SubnetSite::ODD_SUBNET) && TTL == ss2TTL)
                {
                    continue;
                }
                else
                {
                    unsigned char TTLMinus1 = ss2TTL - 1;
                    unsigned char TTLPlus1 = ss2TTL + 1;
                
                    if (TTL == ss2TTL || TTL == TTLMinus1 || (beforeAndAfter && TTL == TTLPlus1))
                        continue;
                    else
                        return false;
                }
            }
            // upperBorder < upperBorder2: hypothetical subnet does not contain ss2
            else
            {
                continue;
            }
        }
    }
    
    return true;
}

SubnetSite *SubnetSiteSet::getShadowSubnet()
{
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        
        if(ss->getStatus() == SubnetSite::SHADOW_SUBNET)
        {
            siteList.erase(i--);
            return ss;
        }
    }
    return NULL;
}

SubnetSite *SubnetSiteSet::getValidSubnet(bool completeRoute)
{
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        
        if(ss->getStatus() == SubnetSite::ACCURATE_SUBNET || 
           ss->getStatus() == SubnetSite::ODD_SUBNET ||
           ss->getStatus() == SubnetSite::SHADOW_SUBNET)
        {
            if(ss->hasValidRoute() && (!completeRoute || ss->hasCompleteRoute()))
            {
                siteList.erase(i--);
                return ss;
            }
        }
    }
    return NULL;
}

unsigned short SubnetSiteSet::getMaximumDistance()
{
    unsigned short longest = 0;
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        unsigned short curDistance = (*i)->getShortestTTL() + 1;
        if(curDistance > longest)
        {
            longest = curDistance;
        }
    }
    return longest;
}

void SubnetSiteSet::sortByRoute()
{
    siteList.sort(SubnetSite::compareRoutes);
}

void SubnetSiteSet::outputAsFile(string filename)
{
    string output = "";
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        string cur = ss->toString();
        
        if(!cur.empty())
            output += cur + "\n";
    }
    
    ofstream newFile;
    newFile.open(filename.c_str());
    newFile << output;
    newFile.close();
    
    // File must be accessible to all
    string path = "./" + filename;
    chmod(path.c_str(), 0766);
}

SubnetSite *SubnetSiteSet::getEncompassingSubnet(InetAddress ip, unsigned char TTL)
{
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss = (*i);
        
        NetworkAddress na = ss->getInferredNetworkAddress();
        InetAddress lowerBorder = na.getLowerBorderAddress();
        InetAddress upperBorder = na.getUpperBorderAddress();
        unsigned char ssTTL = ss->getShortestTTL();
    
        if(ssTTL == TTL)
        {
            if(ip >= lowerBorder && ip <= upperBorder)
            {
                return ss;
            }
        }
    }
    
    return NULL;
}

SubnetSite *SubnetSiteSet::getEncompassingSubnet(SubnetSite *ss)
{
    // Just in case
    if(ss == NULL)
        return NULL;
    
    InetAddress ssLowerBorder, ssUpperBorder;
    unsigned char ssPrefixLength = ss->getInferredSubnetPrefixLength();
    unsigned char ssTTL = ss->getShortestTTL();
    if(ssPrefixLength <= 31)
    {
        NetworkAddress na = ss->getInferredNetworkAddress();
        ssLowerBorder = na.getLowerBorderAddress();
        ssUpperBorder = na.getUpperBorderAddress();
    }
    else
    {
        ssLowerBorder = ss->getInferredSubnetBaseIP();
        ssUpperBorder = ssLowerBorder;
    }
    
    for(list<SubnetSite*>::iterator i = siteList.begin(); i != siteList.end(); ++i)
    {
        SubnetSite *ss2 = (*i);
        NetworkAddress na = ss2->getInferredNetworkAddress();
        InetAddress ss2LowerBorder = na.getLowerBorderAddress();
        InetAddress ss2UpperBorder = na.getUpperBorderAddress();
       
        unsigned char ss2TTL = ss2->getShortestTTL();
        if(ss2TTL == ssTTL)
        {
            if(ssLowerBorder >= ss2LowerBorder && ssLowerBorder <= ss2UpperBorder)
            {
                return ss2;
            }
        }
    }
    
    return NULL;
}
