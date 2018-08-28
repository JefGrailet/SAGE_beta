/*
 * SubnetParser.h
 *
 *  Created on: Mar 27, 2018
 *      Author: jefgrailet
 *
 * This class is dedicated to parsing the subnet dump(s) which are fed to SAGE "Fusion" at 
 * launch. It is derived from a similar class already present in some versions of TreeNET (the 
 * "ancestor" of SAGE), but has been slightly simplified in the process.
 *
 * An option (to be set via Environment singleton) allows to parse subnets and inserts them in the 
 * main subnet set without checking that subnets overlap each other and should be merged. By 
 * default, SAGE "Fusion" does not check this, as the typical usage is to merge datasets that 
 * are obtained by measuring different prefixes.
 *
 * In case of incorrect formatting, error messages will be written to the output stream if the 
 * display mode is set to "slightly verbose".
 *
 * TODO: "multi-route" approach (same prefixes measured from different VPs; if a subnet appears 
 * several times in each dataset, all the routes will be registered)
 */
 
#ifndef SUBNETPARSER_H_
#define SUBNETPARSER_H_

#include <string>
using std::string;

#include "../Environment.h"
#include "../structure/SubnetSite.h"

class SubnetParser
{
public:

    // Constructor, destructor
    SubnetParser(Environment *env);
    ~SubnetParser();
    
    // Parsing method (returns true if a file was indeed parsed)
    bool parse(string inputFileName);

private:
    
    // Pointer to the environment variable
    Environment *env;
    
    // Fields to count, during parsing, correctly parsed subnets along merging and bad parsings
    unsigned int parsedSubnets, credibleSubnets, mergedSubnets, duplicateSubnets, badSubnets;

};

#endif /* SUBNETPARSER_H_ */
