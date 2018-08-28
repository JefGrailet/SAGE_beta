#include <cstdlib>
#include <set>
using std::set;
#include <ctime>
#include <iostream>
using std::cout;
using std::cin;
using std::endl;
using std::exit;
#include <iomanip> // For the display of inferred/refined subnets
using std::left;
using std::setw;
#include <fstream>
using std::ifstream;
using std::ofstream;
using std::ios;
#include <memory>
using std::auto_ptr;
#include <string>
using std::string;
#include <sstream>
using std::stringstream;
#include <list>
using std::list;
#include <algorithm> // For transform() function
#include <getopt.h> // For options parsing
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <ctime> // To obtain current time (for output file)
#include <unistd.h> // For usleep() function

#include "common/inet/InetAddress.h"
#include "common/inet/InetAddressException.h"
#include "common/inet/NetworkAddress.h"
#include "common/inet/NetworkAddressSet.h"
#include "common/inet/InetAddressSet.h"
#include "common/thread/Thread.h"
#include "common/thread/Runnable.h"
#include "common/thread/Mutex.h"
#include "common/thread/ConditionVariable.h"
#include "common/thread/MutexException.h"
#include "common/thread/TimedOutException.h"
#include "common/date/TimeVal.h"
#include "common/utils/StringUtils.h"
#include "common/random/PRNGenerator.h"
#include "common/random/Uniform.h"
#include "prober/icmp/DirectICMPProber.h"

#include "algo/Environment.h"
#include "algo/parsing/ConfigFileParser.h"
#include "algo/parsing/TargetParser.h"
#include "algo/parsing/SubnetParser.h"
#include "algo/graph/GraphBuilder.h"
#include "algo/graph/voyagers/Mariner.h"
#include "algo/graph/voyagers/Galileo.h"
#include "algo/graph/voyagers/Cassini.h"

// Simple function to display usage.

void printInfo()
{
    cout << "Summary\n";
    cout << "=======\n";
    cout << "\n";
    cout << "SAGE (Subnet AGgrEgation) is a topology discovery tool designed to take\n";
    cout << "advantage of subnet inference to unveil, partially or completely, the\n";
    cout << "underlying topology of a target network. To do so, it uses ExploreNET (a\n";
    cout << "subnet discovery tool designed and developed by Ph. D. Mehmet Engin Tozal)\n";
    cout << "to discover subnets within the target network and uses (Paris) traceroute to\n";
    cout << "obtain a route to each of them.\n";
    cout << "\n";
    cout << "The collected data is used to progressively build a graph modeling the target\n";
    cout << "network. To do so, it relies on the notion of \"neighborhood\". A neighborhood\n";
    cout << "is a network location bordered by subnets that are located at at most one hop\n";
    cout << "from each other. In practice, it consists in a single Layer-3 device, or\n";
    cout << "multiple Layer-3 (and possibly Layer-2) devices. Neighborhoods are identified\n";
    cout << "by looking at the end of the route towards each subnet: if the end of each\n";
    cout << "route consists of a same IP, then the final device before reaching the subnets\n";
    cout << "is the same, and therefore it constitutes a neighborhood. Taking a look at\n";
    cout << "specific interfaces from subnets and the neighborhood label (i.e., the final\n";
    cout << "step of each route towards subnets of a same neighborhood) reveals Layer-3\n";
    cout << "(router) interfaces which can be aliased together to discover the routers\n";
    cout << "connecting the subnets.\n";
    cout << "\n";
    cout << "Such an approach was already followed by TreeNET (the ancestor of SAGE), but\n";
    cout << "but the idea of viewing the routes as a tree towards the subnets eventually\n";
    cout << "made it difficult to deepen the topology analysis. SAGE therefore goes beyond\n";
    cout << "the tree idea and applies a thorough algorithm to build a more general graph.\n";
    cout << "SAGE aggregates subnets together on the basis of the end of their route,\n";
    cout << "regardless of the route length, and analyzes the routes of the aggregated\n";
    cout << "subnets to discover peers - other subnet aggregates which the label appears in\n";
    cout << "the routes, meaning these aggregates should be linked together in the graph.\n";
    cout << "To be even more accurate, SAGE uses alias resolution when there are multiple\n";
    cout << "possible peers at a same distance to ensure the IPs belong to separate devices\n";
    cout << "or to a same device. This can lead to having several aggregates actually being\n";
    cout << "part of a same neighborhood in the final graph.\n";
    cout << "\n";
    cout << "There are different versions of SAGE. The program you are currently using is\n";
    cout << "SAGE \"Fusion\", the version which is designed to merge datasets collected at\n";
    cout << "different vantage points to build a single, unified graph of the initial\n";
    cout << "target network.\n";
    cout << "\n";
    cout << "From an algorithmic point of view, SAGE \"Fusion\" only uses a subset of the\n";
    cout << "different algorithmic steps used by SAGE \"Discovery\". Indeed, after parsing\n";
    cout << "the subnet sets, it directly starts the graph building and ends with alias\n";
    cout << "resolution.\n";
    cout << "\n";
    cout << "Algorithmic steps\n";
    cout << "=================\n";
    cout << "\n";
    cout << "0) Launch and subnet parsing\n";
    cout << "----------------------------\n";
    cout << "\n";
    cout << "SAGE parses its main argument to get a list of .subnets files and merges them\n";
    cout << "into a single dataset. A flag can be used to advertise the program it should\n";
    cout << "check if subnets overlap each other. Typically, different pieces of a dataset\n";
    cout << "should be obtained by targetting completely different prefixes, so there\n";
    cout << "should not be overlapping subnets.\n";
    cout << "\n";
    cout << "1) Graph building\n";
    cout << "-----------------\n";
    cout << "\n";
    cout << "SAGE aggregates subnets in function of their neighborhood label, i.e., the\n";
    cout << "last IP on the route before a subnet (if non-anonymous) OR the end sequence\n";
    cout << "if the last hops on the route are anonymous (the sequence going up to the\n";
    cout << "first non-anonymous IP). Once aggregates are formed, SAGE will analyze the\n";
    cout << "routes of the aggregated subnets to find the peers of each aggregate, i.e.,\n";
    cout << "the other aggregates which they connect to (this is inferred when the label\n";
    cout << "of an aggregate appears in the route of a subnet from another aggregate). To\n";
    cout << "ensure accuracy, SAGE conducts a short alias resolution phase when it finds\n";
    cout << "out that there are several peers at a same distance for an aggregate, as\n";
    cout << "it's possible the labels of the peer aggregates are interfaces of a same\n";
    cout << "router, meaning there's actually only one peer (labelled with multiple IPs).\n";
    cout << "After discovering all peers for all aggregates, SAGE post-processes the peers\n";
    cout << "to ensure similar peers (i.e., peers with a least one common label) have the\n";
    cout << "same list of IPs. The final stage of the graph building consists in creating\n";
    cout << "vertices and edges of the graph, and associating a medium (i.e. a subnet,\n";
    cout << "when available) to each edge.\n";
    cout << "\n";
    cout << "Since the graph building of SAGE is independant of the length of each route\n";
    cout << "(as opposed to TreeNET), the algorithm can be run \"as is\" in Fusion after\n";
    cout << "merging the .subnets files into a unique dataset.\n";
    cout << "\n";
    cout << "2) Alias resolution\n";
    cout << "-------------------\n";
    cout << "\n";
    cout << "SAGE processes the graph, vertice by vertice, to conduct alias resolution on\n";
    cout << "each. This step works just like in TreeNET, i.e., it probes a certain amount\n";
    cout << "of times each candidate IP to fingerprint it before picking an alias\n";
    cout << "resolution method and actually aliasing the IPs with each other (when\n";
    cout << "possible). The code of this part is also identical to the one of \"Discovery\".\n";
    cout << "\n";
    cout.flush();
}

void printUsage()
{
    cout << "Usage\n";
    cout << "=====\n";
    cout << "\n";
    cout << "You can use SAGE \"Fusion\" as follows:\n";
    cout << "\n";
    cout << "./sage_fusion [.subnets n°1],[.subnets n°2],[...] -t [target n°1],,[...]\n";
    cout << "\n";
    cout << "where each target can be:\n";
    cout << "-a single IP,\n";
    cout << "-a whole IP block (in CIDR notation),\n";
    cout << "-a file containing a list of the notations mentioned above, which each item\n";
    cout << " being separated with \\n.\n";
    cout << "The .subnets files are typical output files of SAGE \"Discovery\". Note that the\n";
    cout << "targets are mandatory because they are necessary for the computation of some\n";
    cout << "metrics after creating the final graph.\n";
    cout << "\n";
    cout << "You can use various options and flags to handle the main settings, such as the\n";
    cout << "probing protocol, the label of the output files, etc. These options and flags\n";
    cout << "are detailed below. Note, however, that probing or algorithmic parameters can\n";
    cout << "only be handled by providing a separate configuration file. Default parameters\n";
    cout << "will be used if no configuration file is provided.\n";
    cout << "\n";
    cout << "Short   Verbose                             Expected value\n";
    cout << "-----   -------                             --------------\n";
    cout << "\n";
    cout << "-t      --initial-targets                   Set of target prefixes/IPs\n";
    cout << "\n";
    cout << "See usage above for details on how to use this (mandatory) parameter.\n";
    cout << "\n";
    cout << "-c      --configuration-file                Path to a configuration file\n";
    cout << "\n";
    cout << "Use this option to feed a configuration file to SAGE. Use such a file,\n";
    cout << "formatted as key,value pairs (one per line), to edit probing, concurrency or\n";
    cout << "algorithmic parameters. Refer to the documentation of SAGE to learn about the\n";
    cout << "keys you can use and the values you can provide for them.\n";
    cout << "\n";
    cout << "-e      --probing-egress-interface          IP or DNS\n";
    cout << "\n";
    cout << "Interface name through which probing/response packets exit/enter (default is\n";
    cout << "the first non-loopback IPv4 interface in the active interface list). Use this\n";
    cout << "option if your machine has multiple network interface cards and if you want to\n";
    cout << "prefer one interface over the others.\n";
    cout << "\n";
    cout << "-p      --probing-protocol                  \"ICMP\", \"UDP\" or \"TCP\"\n";
    cout << "\n";
    cout << "Use this option to specify the base protocol used to probe target addresses.\n";
    cout << "By default, it is ICMP. Note that some inference techniques (e.g. during alias\n";
    cout << "resolution) rely on a precise protocol which is therefore used instead.\n";
    cout << "\n";
    cout << "WARNING for TCP probing: keep in mind that the TCP probing consists in sending\n";
    cout << "a SYN message to the target, without handling the 3-way handshake properly in\n";
    cout << "case of a SYN+ACK reply. Repeated probes towards a same IP (which can occur\n";
    cout << "during alias resolution) can also be identified as SYN flooding, which is a\n";
    cout << "type of denial-of-service attack. Please consider security issues carefully\n";
    cout << "before using this probing method.\n";
    cout << "\n";
    cout << "-l      --label-output                      String\n";
    cout << "\n";
    cout << "Use this option to edit the label that will be used to name the various output\n";
    cout << "files produced by SAGE (usually ended with a suffix related to their content:\n";
    cout << ".subnets for the list of inferred subnets, .ips for the IP dictionnary with\n";
    cout << "alias resolution hints, etc.). By default, it will use the time at which it\n";
    cout << "first started to run, in the format dd-mm-yyyy hh:mm:ss.\n";
    cout << "\n";
    cout << "-m      --merge-input-subnets               None (Flag)\n";
    cout << "\n";
    cout << "Add this flag to your command-line to request subnet merging when parsing the\n";
    cout << "input .subnets files. In a typical usage scenario, subnets are from different\n";
    cout << "prefixes and the program does not check if subnets overlap each other, but if\n";
    cout << "for some reason such a situation can occur, use this flag to activate subnet\n";
    cout << "merging. Note that this operation can considerably slow down the parsing with\n";
    cout << "large datasets, hence why it is deactivated by default.\n"; 
    cout << "\n";
    cout << "-v      --verbosity                         0, 1, 2 or 3\n";
    cout << "\n";
    cout << "Use this option to handle the verbosity of the console output. Each accepted\n";
    cout << "value corresponds to a \"mode\":\n";
    cout << "\n";
    cout << "* 0: it is the default verbosity. In this mode, only the most significant\n";
    cout << "  details about progress are displayed. For instance, during the (Paris)\n";
    cout << "  traceroute phase, only the subnets for which the route has been measured\n";
    cout << "  are printed, but not the route itself (of course, it is still written in\n";
    cout << "  the .subnets output file).\n";
    cout << "\n";
    cout << "* 1: this is the \"slightly verbose\" mode and it provides more details on the\n";
    cout << "  progress. For instance, the routes obtained during the traceroute phase are\n";
    cout << "  now printed, as well as the different steps of the alias resolution hint\n";
    cout << "  collection process.\n";
    cout << "\n";
    cout << "* 2: stacking up on the previous mode, this last mode also dumps small logs\n";
    cout << "  for each probe once it has been carried out. It is equivalent to a debug\n";
    cout << "  mode.\n";
    cout << "\n";
    cout << "  WARNING: the debug mode is extremely verbose and should only be considered\n";
    cout << "  for unusual situations where investigating the result of each probe becomes\n";
    cout << "  necessary. Do not use it for large-scale work, as redirecting the console\n";
    cout << "  output to files might produce unnecessarily large files.\n";
    cout << "\n";
    cout << "-k      --external-logs                     None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to place the probing logs in separate\n";
    cout << "output files rather than having them in the main console output. The goal of\n";
    cout << "this feature is to allow the user to have a short summary of the execution at\n";
    cout << "the end (to learn quickly elapsed time for each phase, amount of probes, etc.)\n";
    cout << "while the probing details remain accessible in separate files.\n";
    cout << "\n";
    cout << "Given [label], the label used for output files (either edited with -l or set\n";
    cout << "by default to dd-mm-yyyy hh:mm:ss), the logs will be named Log_[label]_[phase]\n";
    cout << "where [phase] is either: pre-scanning, scanning, traceroute, neighborhoods or\n";
    cout << "alias_resolution.\n";
    cout << "\n";
    cout << "-h      --help                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to know how to use SAGE and see the\n";
    cout << "complete list of options and flags and how they work. It will not run further\n";
    cout << "after displaying this, though the -i flag can be used in addition.\n";
    cout << "\n";
    cout << "-i      --info                              None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to read a summary of how SAGE works,\n";
    cout << "with a list of the different algorithm steps. Keep in mind, however, that this\n";
    cout << "is only a summary and that you will learn much more on the way SAGE works by\n";
    cout << "reading related papers or the source code. SAGE will not run further after\n";
    cout << "displaying this, though -c and -h flags can be used in addition.\n";
    cout << "\n";
    cout << "About\n";
    cout << "=====\n";
    cout << "\n";
    cout << "SAGE (Subnet AGgrEgation) v1.0 is a subnet-based topology discovery tool,\n";
    cout << "initially designed and implemented by Jean-François Grailet (Ph. D. student\n";
    cout << "at the Research Unit in Networking, Montefiore Institute, University of Liège,\n";
    cout << "Belgium) in 2017-2018.\n";
    cout << "\n";
    cout << "SAGE comes as several pieces of software. This program is the \"Fusion\"\n";
    cout << "version, i.e., the program used to merge datasets obtained by individual\n";
    cout << "instances of the \"Discovery\" version.\n";
    cout << "\n";
    cout << "Large parts of SAGE are inherited from the following software:\n";
    cout << "-TreeNET v3.3, written by Jean-François Grailet (last updated in 08/2017),\n";
    cout << "-ExploreNET v2.1, written by Mehmet Engin Tozal (2013).\n";
    cout << "It's worth noting TreeNET is also initially derived from ExploreNET v2.1.\n";
    cout << "\n";
    
    cout.flush();
}

// Simple function to get the current time as a string object.

string getCurrentTimeStr()
{
    time_t rawTime;
    struct tm *timeInfo;
    char buffer[80];

    time(&rawTime);
    timeInfo = localtime(&rawTime);

    strftime(buffer, 80, "%d-%m-%Y %T", timeInfo);
    string timeStr(buffer);
    
    return timeStr;
}

// Simple function to convert an elapsed time (in seconds) into days/hours/mins/secs format

string elapsedTimeStr(unsigned long elapsedSeconds)
{
    if(elapsedSeconds == 0)
    {
        return "less than one second";
    }

    unsigned short secs = (unsigned short) elapsedSeconds % 60;
    unsigned short mins = (unsigned short) (elapsedSeconds / 60) % 60;
    unsigned short hours = (unsigned short) (elapsedSeconds / 3600) % 24;
    unsigned short days = (unsigned short) elapsedSeconds / 86400;
    
    stringstream ss;
    if(days > 0)
    {
        if(days > 1)
            ss << days << " days ";
        else
            ss << "1 day ";
    }
    if(hours > 0)
    {
        if(hours > 1)
            ss << hours << " hours ";
        else
            ss << "1 hour ";
    }
    if(mins > 0)
    {
        if(mins > 1)
            ss << mins << " minutes ";
        else
            ss << "1 minute ";
    }
    if(secs > 1)
        ss << secs << " seconds";
    else
        ss << secs << " second";
    
    return ss.str();    
}

// Main function; deals with inputs and launches thread(s)

int main(int argc, char *argv[])
{
    // Default parameters for options which can be edited by user
    string configFilePath = "";
    string targetsStr = "";
    InetAddress localIPAddress;
    unsigned char LANSubnetMask = 0;
    bool mergeInputSubnets = false;
    unsigned short probingProtocol = Environment::PROBING_PROTOCOL_ICMP;
    unsigned short displayMode = Environment::DISPLAY_MODE_LACONIC;
    bool externalLogs = false;
    string outputFileName = ""; // Gets a default value later if not set by user.
    
    // Values to check if algorithmic summary or usage should be displayed.
    bool displayInfo = false, displayUsage = false;
    
    /*
     * PARSING ARGUMENT
     * 
     * The main argument (target prefixes or input files, each time separated by commas) can be 
     * located anywhere. To make things simple for getopt_long(), argv is processed to find it and 
     * put it at the end. If not found, SAGE stops and displays an error message.
     */
    
    int totalArgs = argc;
    string subnetsStr = ""; // List of .subnets files
    bool found = false;
    bool flagParam = false; // True if a value for a parameter is expected
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'h':
                case 'i':
                case 'k':
                case 'm':
                    break;
                default:
                    flagParam = true;
                    break;
            }
        }
        else if(flagParam)
        {
            flagParam = false;
        }
        else
        {
            // Argument found!
            char *arg = argv[i];
            for(int j = i; j < argc - 1; j++)
                argv[j] = argv[j + 1];
            argv[argc - 1] = arg;
            found = true;
            totalArgs--;
            break;
        }
    }
    
    subnetsStr = argv[argc - 1];
    
    /*
     * PARSING PARAMETERS
     *
     * In addition to the main argument parsed above, this program provides various input flags 
     * which can be used to handle some essential parameters. Probing, algorithmic or concurrency 
     * parameters must be edited via a configuration file (see ConfigFileParser, Environment 
     * classes).
     */
     
    int opt = 0;
    int longIndex = 0;
    const char* const shortOpts = "c:e:hikl:mp:t:v:";
    const struct option longOpts[] = {
            {"initial-targets", required_argument, NULL, 't'}, 
            {"configuration-file", required_argument, NULL, 'c'}, 
            {"probing-egress-interface", required_argument, NULL, 'e'}, 
            {"probing-protocol", required_argument, NULL, 'p'}, 
            {"label-output", required_argument, NULL, 'l'}, 
            {"merge-input-subnets", no_argument, NULL, 'm'}, 
            {"verbosity", required_argument, NULL, 'v'}, 
            {"external-logs", no_argument, NULL, 'k'}, 
            {"help", no_argument, NULL, 'h'}, 
            {"info", no_argument, NULL, 'i'},
            {NULL, 0, NULL, 0}
    };
    
    string optargSTR;
    try
    {
        while((opt = getopt_long(totalArgs, argv, shortOpts, longOpts, &longIndex)) != -1)
        {
            /*
             * Beware: use the line optargSTR = string(optarg); ONLY for flags WITH arguments !! 
             * Otherwise, it prevents the code from recognizing flags like -v, -h or -g (because 
             * they require no argument) and make it throw an exception... To avoid this, a second 
             * switch is used.
             *
             * (this error is still present in ExploreNET v2.1)
             */
            
            switch(opt)
            {
                case 'h':
                case 'i':
                case 'k':
                case 'm':
                    break;
                default:
                    optargSTR = string(optarg);
                    
                    /*
                     * For future readers: optarg is of type extern char*, and is defined in getopt.h.
                     * Therefore, you will not find the declaration of this variable in this file.
                     */
                    
                    break;
            }
            
            // Now we can actually treat the options.
            int gotNb = 0;
            switch(opt)
            {
                case 't':
                    targetsStr = optargSTR;
                    break;
                case 'c':
                    configFilePath = optargSTR;
                    break;
                case 'e':
                    try
                    {
                        localIPAddress = InetAddress::getLocalAddressByInterfaceName(optargSTR);
                    }
                    catch (InetAddressException &e)
                    {
                        cout << "Error for -e option: cannot obtain any IP address ";
                        cout << "assigned to the interface \"" + optargSTR + "\". ";
                        cout << "Please fix the argument for this option before ";
                        cout << "restarting SAGE.\n" << endl;
                        return 1;
                    }
                    break;
                case 'p':
                    std::transform(optargSTR.begin(), optargSTR.end(), optargSTR.begin(),::toupper);
                    if(optargSTR == string("UDP"))
                    {
                        probingProtocol = Environment::PROBING_PROTOCOL_UDP;
                    }
                    else if(optargSTR == string("TCP"))
                    {
                        probingProtocol = Environment::PROBING_PROTOCOL_TCP;
                    }
                    else if(optargSTR != string("ICMP"))
                    {
                        cout << "Warning for option -b: unrecognized protocol " << optargSTR;
                        cout << ". Please select a protocol between the following three: ";
                        cout << "ICMP, UDP and TCP. Note that ICMP is the default base ";
                        cout << "protocol.\n" << endl;
                    }
                    break;
                case 'l':
                    outputFileName = optargSTR;
                    break;
                case 'm':
                    mergeInputSubnets = true;
                    break;
                case 'v':
                    gotNb = std::atoi(optargSTR.c_str());
                    if(gotNb >= 0 && gotNb <= 2)
                        displayMode = (unsigned short) gotNb;
                    else
                    {
                        cout << "Warning for -v option: an unrecognized mode (i.e., value ";
                        cout << "out of [0,2]) was provided. SAGE will use the laconic ";
                        cout << "mode (default mode).\n" << endl;
                    }
                    break;
                case 'k':
                    externalLogs = true;
                    break;
                case 'h':
                    displayUsage = true;
                    break;
                case 'i':
                    displayInfo = true;
                    break;
                default:
                    break;
            }
        }
    }
    catch(std::logic_error &le)
    {
        cout << "Use -h or --help to get more details on how to use SAGE." << endl;
        return 1;
    }
    
    if(displayInfo || displayUsage)
    {
        if(displayInfo)
            printInfo();
        if(displayUsage)
            printUsage();
        return 0;
    }
    
    if(!found)
    {
        cout << "No target prefix or target file was provided. Use -h or --help to get more ";
        cout << "details on how to use SAGE." << endl;
        return 0;
    }
    
    /*
     * SETTING THE ENVIRONMENT
     *
     * Before listing target IPs, the initialization is completed by getting the local IP and the 
     * local subnet mask and creating a Environment object, a singleton which will be passed by 
     * pointer to other classes of SAGE to be able to get all the current settings, which are 
     * either default values either values parsed in the parameters provided by the user. The 
     * singleton also provides access to data structures other classes should be able to access.
     */

    if(localIPAddress.isUnset())
    {
        try
        {
            localIPAddress = InetAddress::getFirstLocalAddress();
        }
        catch(InetAddressException &e)
        {
            cout << "Cannot obtain a valid local IP address for probing. ";
            cout << "Please check your connectivity." << endl;
            return 1;
        }
    }

    if(LANSubnetMask == 0)
    {
        try
        {
            LANSubnetMask = NetworkAddress::getLocalSubnetPrefixLengthByLocalAddress(localIPAddress);
        }
        catch(InetAddressException &e)
        {
            cout << "Cannot obtain subnet mask of the local area network (LAN) .";
            cout << "Please check your connectivity." << endl;
            return 1;
        }
    }

    NetworkAddress LAN(localIPAddress, LANSubnetMask);
    
    /*
     * We determine now the label of the output files. Here, it is either provided by the user 
     * (via -l flag), either it is set to the current date (dd-mm-yyyy hh:mm:ss).
     */
    
    string newFileName = "";
    if(outputFileName.length() > 0)
        newFileName = outputFileName;
    else
        newFileName = getCurrentTimeStr();
    
    // Initialization of the Environment singleton
    Environment *env = new Environment(&cout, 
                                       externalLogs, 
                                       probingProtocol, 
                                       mergeInputSubnets, 
                                       localIPAddress, 
                                       LAN, 
                                       displayMode);
    
    // Parses config file, if provided
    if(configFilePath.length() > 0)
    {
        ConfigFileParser *parser = new ConfigFileParser(env);
        parser->parse(configFilePath);
        delete parser;
    }
    
    /*
     * The code now checks if it can open a socket at all to properly advertise the user should 
     * use "sudo" or "su". Not putting this step would result in SAGE scheduling probing work 
     * (pre-scanning) and immediately trigger emergency stop (which should only occur when, after 
     * doing some probing work, software resources start lacking), which is not very elegant.
     */
    
    try
    {
        DirectProber *test = new DirectICMPProber(env->getAttentionMessage(), 
                                                  env->getTimeoutPeriod(), 
                                                  env->getProbeRegulatingPeriod(), 
                                                  DirectICMPProber::DEFAULT_LOWER_ICMP_IDENTIFIER, 
                                                  DirectICMPProber::DEFAULT_UPPER_ICMP_IDENTIFIER, 
                                                  DirectICMPProber::DEFAULT_LOWER_ICMP_SEQUENCE, 
                                                  DirectICMPProber::DEFAULT_UPPER_ICMP_SEQUENCE, 
                                                  false);
        
        delete test;
    }
    catch(SocketException &e)
    {
        cout << "Unable to create sockets. Try running this program as a privileged user (for ";
        cout << "example, try with sudo)." << endl;
        delete env;
        return 1;
    }
    
    // Gets direct access to the subnet set
    SubnetSiteSet *subnetSet = env->getSubnetSet();
    
    /*
     * INPUT FILE PARSING
     *
     * The provided input file(s) (either a single subnet dump, either a pair subnet dump/IP 
     * dictionnary dump) are read and parsed into the subnet set and the IP dictionnary of the 
     * current instance of TreeNET "Forester".
     */
    
    // Parsing targets
    TargetParser *parser = new TargetParser(env);
    parser->parseCommandLine(targetsStr);
    delete parser;
    if(env->getTotalIPsInitialTargets() == 0)
    {
        cout << "Please input valid targets (with -t) to continue.\n" << endl;
        delete env;
        return 1;
    }

    // Listing input .subnets files
    list<string> filePaths; // Needs to be declared here for later
    
    size_t pos = subnetsStr.find(',');
    if(pos != std::string::npos)
    {
        // Listing all file paths
        std::stringstream ss(subnetsStr);
        std::string inputFileStr;
        while (std::getline(ss, inputFileStr, ','))
            filePaths.push_back(inputFileStr);
    
        // Checking that each file exists
        for(list<string>::iterator it = filePaths.begin(); it != filePaths.end(); ++it)
        {
            // Subnet dump
            ifstream inFile;
            inFile.open(((*it)).c_str());
            if(inFile.is_open())
            {
                inFile.close();
            }
            else
            {
                cout << "No " << (*it) << " to parse.\n" << endl;
                filePaths.erase(it--);
            }
        }
        
        if(filePaths.size() == 1)
        {
            cout << "Multiple input files were provided, but only one of them actually ";
            cout << "exists in the file system. SAGE will not run further.\n" << endl;
            delete env;
            return 1;
        }
        else if(filePaths.size() == 0)
        {
            cout << "Please input existing .subnets files to continue.\n" << endl;
            delete env;
            return 1;
        }
    }
    else
    {
        cout << "Please provide more than one .subnets file to continue.\n" << endl;
        delete env;
        return 1;
    }
    
    // Single input file: both .subnet and .ip dumps are parsed
    SubnetParser *sp = new SubnetParser(env);
    
    cout << "--- Start of input files parsing ---" << endl;
    timeval parsingStart, parsingEnd;
    gettimeofday(&parsingStart, NULL);
    
    if(externalLogs)
        env->openLogStream("Log_" + newFileName + "_parsing");
    
    for(list<string>::iterator it = filePaths.begin(); it != filePaths.end(); ++it)
    {
        string subnetsPath = (*it);
        bool parsingResult = sp->parse(subnetsPath);
        if(!parsingResult)
        {
            ostream *out = &cout;
            if(externalLogs)
                out = env->getOutputStream();
            (*out) << "Could not parse " << subnetsPath << "." << endl;
        }
    }
    
    delete sp;
    
    env->fillIPDictionnary();
    
    if(subnetSet->getNbSubnets() == 0)
    {
        ostream *out = &cout;
        if(externalLogs)
            out = env->getOutputStream();
        (*out) << "Could not parse any subnet at all. SAGE \"Fusion\" will stop here." << endl;
        delete env;
        return 0;
    }
    subnetSet->sortSet();
    
    if(externalLogs)
        env->closeLogStream();
    
    cout << "--- End of input files parsing (" << getCurrentTimeStr() << ") ---" << endl;
    gettimeofday(&parsingEnd, NULL);
    unsigned long parsingElapsed = parsingEnd.tv_sec - parsingStart.tv_sec;
    cout << "Elapsed time: " << elapsedTimeStr(parsingElapsed) << "\n" << endl;
    
    subnetSet->outputAsFile(newFileName + ".subnets");
    cout << "Final set of subnets has been written in a new output file ";
    cout << newFileName << ".subnets.\n" << endl;

    // Various variables/structures which should be considered when catching some exception
    GraphBuilder *builder = NULL;
    Galileo *galileo = NULL;
    
    try
    {
        /*
         * STEP II: GRAPH BUILDING
         *
         * With the parsed subnets and their routes, SAGE "Fusion" has all the important data to 
         * start building the graph modeling the target network. It works just like in Discovery: 
         * it first starts by deriving the neighborhood label of each subnet from their route 
         * (the neighborhood label being either the last IP on the route, either a sequence of 
         * one non-anonymous IP followed by anomalies like anonymous IPs or cycling hops), then 
         * aggregates subnets when they share the same neighborhood label. After aggregating the 
         * subnets, it infers the peers of each aggregate, i.e., the other aggregates they are 
         * connected to. This is inferred by checking which IPs in the routes of the aggregated 
         * subnets belong to other aggregates. When an aggregate displays several possible peers 
         * at the same distance, it conducts alias resolution on their label to see if these 
         * peers are from different devices or from a same machine (meaning each aggregate is 
         * actually a chunk of a larger neighborhood). Peer post-processing and edge creation 
         * complete this operation. For more details, check the code and comments of the 
         * GraphBuilding class.
         */
        
        cout << "--- Start of graph building ---" << endl;
        timeval graphBuildingStart, graphBuildingEnd;
        gettimeofday(&graphBuildingStart, NULL);
        
        if(externalLogs)
            env->openLogStream("Log_" + newFileName + "_graph");
        
        builder = new GraphBuilder(env);
        builder->build();
        
        Graph *g = builder->getResult();
        
        if(builder->gotAnomalies())
            builder->outputAnomalies("Log_" + newFileName + "_graph_anomalies");
        
        delete builder;
        builder = NULL;
        
        if(externalLogs)
            env->closeLogStream();
        
        cout << "--- End of graph building (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&graphBuildingEnd, NULL);
        unsigned long graphBuildingElapsed = graphBuildingEnd.tv_sec - graphBuildingStart.tv_sec;
        cout << "Elapsed time: " << elapsedTimeStr(graphBuildingElapsed) << endl;
        if(env->getTotalProbes() > 0)
        {
            double successRate = ((double) env->getTotalSuccessfulProbes() / (double) env->getTotalProbes()) * 100;
            cout << "Total amount of probes (alias resolution): " << env->getTotalProbes() << endl;
            cout << "Total amount of successful probes: " << env->getTotalSuccessfulProbes();
            cout << " (" << successRate << "%)\n" << endl;
        }
        else
            cout << "There was no need for using additionnal probes (alias resolution)." << endl;
        env->resetProbeAmounts();
        
        // Handles the graph
        Mariner *m = new Mariner(env);
        
        m->visit(g);
        m->outputNeighborhoods(newFileName + ".neighborhoods");
        m->outputGraph(newFileName + ".graph");
        
        cout << "Discovered neighborhoods have been written in a new output file ";
        cout << newFileName << ".neighborhoods." << endl;
        cout << "The graph has been written in a new output file ";
        cout << newFileName << ".graph.\n" << endl;
        
        if(newFileName == "Test")
        {
            cout << "Quitting for test purposes." << endl;
            
            m->cleanNeighborhoods();
            
            delete m;
            delete g;
            delete env;
            return 0;
        }
        
        /*
         * STEP III: ALIAS RESOLUTION
         *
         * Now that a graph is built and that neighborhoods are complete, the full alias 
         * resolution can take place. Just like in TreeNET, neighborhoods are visited one by one 
         * and the IPs likely to be interfaces of a Layer-3 device (most likely a router) are 
         * listed, then probed for fingerprinting. Similar IPs are then grouped together and 
         * actually aliased together (when possible).
         */
        
        cout << "--- Start of alias resolution ---" << endl;
        timeval aliasResoStart, aliasResoEnd;
        gettimeofday(&aliasResoStart, NULL);
        
        if(externalLogs)
            env->openLogStream("Log_" + newFileName + "_alias_resolution");
        
        galileo = new Galileo(env);
        galileo->visit(g);
        
        if(externalLogs)
            env->closeLogStream();
        
        galileo->figaro(newFileName + ".aliases");
        galileo->magnifico(newFileName + ".fingerprints");
        
        delete galileo;
        galileo = NULL;
        
        cout << "--- End of alias resolution (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&aliasResoEnd, NULL);
        unsigned long aliasResoElapsed = aliasResoEnd.tv_sec - aliasResoStart.tv_sec;
        double successRate = ((double) env->getTotalSuccessfulProbes() / (double) env->getTotalProbes()) * 100;
        cout << "Elapsed time: " << elapsedTimeStr(aliasResoElapsed) << endl;
        cout << "Total amount of probes: " << env->getTotalProbes() << endl;
        cout << "Total amount of successful probes: " << env->getTotalSuccessfulProbes();
        cout << " (" << successRate << "%)\n" << endl;
        env->resetProbeAmounts();
        
        cout << "Discovered aliases have been written in a new output file ";
        cout << newFileName << ".aliases." << endl;
        cout << "Fingerprints of the IPs involved in alias resolution have been written ";
        cout << "in a new output file " << newFileName << ".fingerprints." << endl;
        
        env->getIPTable()->outputDictionnary(newFileName + ".ips");
        cout << "IP dictionnary with alias resolution hints has been saved in an output file ";
        cout << newFileName << ".ips.\n";
        
        Cassini *c = new Cassini(env);
        
        c->visit(g);
        c->outputMetrics(newFileName + ".metrics");
        
        delete c;
        
        cout << "Metrics on the graph have been written in a new output file " << newFileName;
        cout << ".metrics." << endl;
        
        m->cleanNeighborhoods();
        
        delete m;
        delete g;
        
    }
    catch(StopException e)
    {
        cout << "Program is halting now." << endl;
    
        IPLookUpTable *dict = env->getIPTable();
        if(!dict->isEmpty())
            cout << endl;
        
        // Emergency save of the IP dictionnary
        if(!dict->isEmpty())
        {
            dict->outputDictionnary("[Stopped] " + newFileName + ".ips");
            cout << "IP dictionnary has been saved in a file \"[Stopped] "+ newFileName + ".ips\"." << endl;
        }
        
        // Because pointers are set to NULL after deletion, next lines should not cause any issue.
        delete builder;
        delete galileo;
        
        delete env;
        return 1;
    }
    
    delete env;
    return 0;
}
