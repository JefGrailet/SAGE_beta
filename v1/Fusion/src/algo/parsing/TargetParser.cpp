/*
 * TargetParser.cpp
 *
 *  Created on: Oct 2, 2015
 *      Author: jefgrailet
 *
 * Implements the class defined in TargetParser.h.
 */

#include <cstdlib>
#include <algorithm>
#include <sstream>
#include <vector>
#include <fstream>

#include "TargetParser.h"

TargetParser::TargetParser(Environment *env)
{
    this->env = env;
}

TargetParser::~TargetParser()
{
}

void TargetParser::parse(string input, char separator)
{
    if(input.size() == 0)
    {
        return;
    }
    
    std::stringstream ss(input);
    std::string targetStr;
    list<InetAddress> *itIPs = env->getInitialTargetIPs();
    list<NetworkAddress> *itRanges = env->getInitialTargetRanges();
    while (std::getline(ss, targetStr, separator))
    {
        // Ignore too small strings
        if (targetStr.size() < MIN_LENGTH_TARGET_STR)
            continue;
    
        size_t pos = targetStr.find('/');
        // Target is a single IP
        if(pos == std::string::npos)
        {
            InetAddress target;
            try
            {
                target.setInetAddress(targetStr);
                parsedIPs.push_back(target);
                itIPs->push_back(target);
            }
            catch (InetAddressException &e)
            {
                ostream *out = env->getOutputStream();
                (*out) << "Malformed/Unrecognized destination IP address or host name \"" + targetStr + "\"" << endl;
                continue;
            }
        }
        // Target is a whole address block
        else
        {
            std::string prefix = targetStr.substr(0, pos);
            unsigned char prefixLength = (unsigned char) std::atoi(targetStr.substr(pos + 1).c_str());
            try
            {
                InetAddress blockPrefix(prefix);
                NetworkAddress block(blockPrefix, prefixLength);
                parsedIPBlocks.push_back(block);
                itRanges->push_back(block);
            }
            catch (InetAddressException &e)
            {
                ostream *out = env->getOutputStream();
                (*out) << "Malformed/Unrecognized address block \"" + targetStr + "\"" << endl;
                continue;
            }
        }
    }
}

void TargetParser::parseCommandLine(string targetListStr)
{
    std::stringstream ss(targetListStr);
    std::string targetStr;
    std::string plainTargetsStr = "";
    bool first = true;
    while (std::getline(ss, targetStr, ','))
    {
        // Tests if this is some file
        std::ifstream inFile;
        inFile.open(targetStr.c_str());
        
        if(inFile.is_open())
        {
            // Parses the input file
            string inputFileContent = "";
            inputFileContent.assign((std::istreambuf_iterator<char>(inFile)),
                                    (std::istreambuf_iterator<char>()));
            
            parse(inputFileContent, '\n');
        }
        else
        {
            if(first)
                first = false;
            else
                plainTargetsStr += ',';
        
            plainTargetsStr += targetStr;
        }
    }

    // Parses remaining (and plain) targets
    parse(plainTargetsStr, ',');
}

void TargetParser::removeDuplicata(list<InetAddress> *lIPs, 
                                   list<NetworkAddress> *lIPBlocks, 
                                   NetworkAddress block)
{
    InetAddress lowerBound = block.getLowerBorderAddress();
    InetAddress upperBound = block.getUpperBorderAddress();
    
    for(std::list<InetAddress>::iterator it = lIPs->begin(); it != lIPs->end(); ++it)
    {
        InetAddress cur = (*it);
    
        if(cur >= lowerBound && cur <= upperBound)
        {
            lIPs->erase(it--);
        }
    }
    
    for(std::list<NetworkAddress>::iterator it = lIPBlocks->begin(); it != lIPBlocks->end(); ++it)
    {
        NetworkAddress cur = (*it);
        
        InetAddress curLowerBound = cur.getLowerBorderAddress();
        InetAddress curUpperBound = cur.getUpperBorderAddress();
    
        if(curLowerBound >= lowerBound && curUpperBound <= upperBound)
        {
            lIPBlocks->erase(it--);
        }
    }
}
