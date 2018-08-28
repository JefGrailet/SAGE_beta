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
using std::ofstream;
using std::ios;
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
#include "algo/utils/ConfigFileParser.h"
#include "algo/utils/TargetParser.h"
#include "algo/prescanning/NetworkPrescanner.h"
#include "algo/scanning/NetworkScanner.h"
#include "algo/traceroute/SubnetTracer.h"
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
    cout << "SAGE \"Discovery\", the version that can conduct a full measurement of a target\n";
    cout << "network and output a graph of it, among others.\n";
    cout << "\n";
    cout << "From an algorithmic point of view, SAGE \"Discovery\" consists in a sequence of\n";
    cout << "different steps. While other resources provide much more details about how\n";
    cout << "SAGE works, a \"big picture\" of the different steps is provided below. It is\n";
    cout << "worth noting that SAGE is currently only available for IPv4, as moving to IPv6\n";
    cout << "requires a working IPv6 subnet inference algorithm or another method of\n";
    cout << "aggregating several entities together.\n";
    cout << "\n";
    cout << "Algorithmic steps\n";
    cout << "=================\n";
    cout << "\n";
    cout << "0) Launch and target selection\n";
    cout << "------------------------------\n";
    cout << "\n";
    cout << "SAGE parses its main argument to get a list of all the target IPs it should\n";
    cout << "consider in the next steps.\n";
    cout << "\n";
    cout << "1) Network pre-scanning\n";
    cout << "-----------------------\n";
    cout << "\n";
    cout << "Each target IP is probed once to evaluate its liveness. Unresponsive IPs are\n";
    cout << "probed a second time with twice the initial timeout as a second opinion (note:\n";
    cout << "a third opinion can be requested via a configuration file). Only IPs that were\n";
    cout << "responsive during this step will be probed again during the next steps,\n";
    cout << "avoiding useless probing work. Several target IPs are probed at the same time,\n";
    cout << "via multi-threading.\n";
    cout << "\n";
    cout << "2) Network scanning\n";
    cout << "-------------------\n";
    cout << "\n";
    cout << "Responsive target IPs are analyzed with ExploreNET, with multi-threading to\n";
    cout << "speed up the process. After all current threads completed and before the next\n";
    cout << "set of threads, SAGE performs refinement on the new subnets to evaluate and\n";
    cout << "\"fix\" them when possible, just like in TreeNET. If one target IP is already\n";
    cout << "encompassed by a previously inferred subnet, it is not investigated further.\n";
    cout << "\n";
    cout << "3) Paris traceroute\n";
    cout << "-------------------\n";
    cout << "\n";
    cout << "SAGE runs Paris traceroute towards each inferred (and refined) subnet to get\n";
    cout << "their respective route. Again, multi-threading is used to speed up the whole\n";
    cout << "process, with several small delays between threads or probes to avoid being\n";
    cout << "too aggressive towards the target network. A short refinement (\"repairment\")\n";
    cout << "takes place at the end of this phase to try to fix routes featuring anonymous\n";
    cout << "hops. Such hops are fixed by comparing the incomplete route with similar but\n";
    cout << "complete routes and replacing a sequence A, 0, C (0 for anonymous) with a\n";
    cout << "sequence A, B, C as long as B is the only possibility in other routes.\n";
    cout << "\n";
    cout << "4) Graph building\n";
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
    cout << "5) Alias resolution\n";
    cout << "-------------------\n";
    cout << "\n";
    cout << "SAGE processes the graph, vertice by vertice, to conduct alias resolution on\n";
    cout << "each. This step works just like in TreeNET, i.e., it probes a certain amount\n";
    cout << "of times each candidate IP to fingerprint it before picking an alias\n";
    cout << "resolution method and actually aliasing the IPs with each other (when\n";
    cout << "possible).\n";
    cout << "\n";
    cout.flush();
}

void printUsage()
{
    cout << "Usage\n";
    cout << "=====\n";
    cout << "\n";
    cout << "You can use SAGE \"Discovery\" as follows:\n";
    cout << "\n";
    cout << "./sage [target n°1],[target n°2],[...]\n";
    cout << "\n";
    cout << "where each target can be:\n";
    cout << "-a single IP,\n";
    cout << "-a whole IP block (in CIDR notation),\n";
    cout << "-a file containing a list of the notations mentioned above, which each item\n";
    cout << " being separated with \\n.\n";
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
    cout << "-j      --join-explorenet-records           None (flag)\n";
    cout << "\n";
    cout << "Add this flag to your command line to get an additionnal output file, ending\n";
    cout << "with the extension .xnet, which will list target IPs along the subnets that\n";
    cout << "were obtained from them during the inference (BEFORE refinement). Moreover,\n";
    cout << "the subnet positioning cost (SPC), i.e. amount of probes to position the\n";
    cout << "subnet, the subnet inference cost (SIC), i.e. amount of probes to infer this\n";
    cout << "subnet, and the alternative subnet, if any, are also provided. These pieces of\n";
    cout << "info are normally provided in ExploreNET v2.1 but eventually disappeared from\n";
    cout << "the regular output of the direct ancestor of SAGE (TreeNET). This flag allows\n";
    cout << "you to get these details in addition to the typical output files, which can be\n";
    cout << "meaningful in some situations.\n";
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
    cout << "* 2: this mode stacks up on the previous and adds more details about the\n";
    cout << "  subnet inference as carried out by ExploreNET (before refinement) by dumping\n";
    cout << "  in the console a short log from a thread that inferred a subnet from a given\n";
    cout << "  target IP.\n";
    cout << "\n";
    cout << "* 3: again stacking up on the previous, this last mode also dumps small logs\n";
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
    cout << "SAGE comes as several pieces of software. This program is the \"Discovery\"\n";
    cout << "version, i.e., the program used to fully probe a network and output a graph\n";
    cout << "based on the measurements, using a few prefixes as input.\n";
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
    InetAddress localIPAddress;
    unsigned char LANSubnetMask = 0;
    unsigned short probingProtocol = Environment::PROBING_PROTOCOL_ICMP;
    bool saveXNETRecords = false;
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
    string targetsStr = ""; // List of targets (input files or plain targets)
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
                case 'j':
                case 'k':
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
    
    targetsStr = argv[argc - 1];
    
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
    const char* const shortOpts = "c:e:hijkl:p:v:";
    const struct option longOpts[] = {
            {"configuration-file", required_argument, NULL, 'c'}, 
            {"probing-egress-interface", required_argument, NULL, 'e'}, 
            {"probing-protocol", required_argument, NULL, 'p'}, 
            {"label-output", required_argument, NULL, 'l'}, 
            {"join-explorenet-records", no_argument, NULL, 'j'}, 
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
                case 'j':
                case 'k':
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
                case 'j':
                    saveXNETRecords = true;
                    break;
                case 'v':
                    gotNb = std::atoi(optargSTR.c_str());
                    if(gotNb >= 0 && gotNb <= 3)
                        displayMode = (unsigned short) gotNb;
                    else
                    {
                        cout << "Warning for -v option: an unrecognized mode (i.e., value ";
                        cout << "out of [0,3]) was provided. SAGE will use the laconic ";
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
                                       saveXNETRecords, 
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

    // Various variables/structures which should be considered when catching some exception
    TargetParser *parser = NULL;
    NetworkPrescanner *prescanner = NULL;
    NetworkScanner *scanner = NULL;
    SubnetTracer *tracer = NULL;
    GraphBuilder *builder = NULL;
    Galileo *galileo = NULL;
    
    bool outputtedSubnets = false;
    
    try
    {
        // Parses inputs and gets target lists
        parser = new TargetParser(env);
        parser->parseCommandLine(targetsStr);
        
        cout << "SAGE (Subnet AGgrEgation) \"Discovery\" v1.0  - Time at start: ";
        cout << getCurrentTimeStr() << "\n" << endl;
        
        // Announces that it will ignore LAN.
        if(parser->targetsEncompassLAN())
        {
            cout << "Target IPs encompass the LAN of the vantage point ("
                 << LAN.getSubnetPrefix() << "/" << (unsigned short) LAN.getPrefixLength() 
                 << "). IPs belonging to the LAN will be ignored.\n" << endl;
        }

        list<InetAddress> targetsPrescanning = parser->getTargetsPrescanning();

        // Stops if no target at all
        if(targetsPrescanning.size() == 0)
        {
            cout << "No target to probe." << endl;
            delete parser;
            parser = NULL;
            
            cout << "Use \"--help\" or \"-h\" parameter to reach help" << endl;
            delete env;
            return 1;
        }
        
        /*
         * STEP I: NETWORK PRE-SCANNING
         *
         * Each address from the set of (re-ordered) target addresses are probed to check that 
         * they are live IPs.
         */
        
        prescanner = new NetworkPrescanner(env);
        prescanner->setTimeoutPeriod(env->getTimeoutPeriod());
        
        cout << "--- Start of network pre-scanning ---" << endl;
        timeval prescanningStart, prescanningEnd;
        gettimeofday(&prescanningStart, NULL);
        
        if(externalLogs)
            env->openLogStream("Log_" + newFileName + "_pre-scanning");
        
        prescanner->run(targetsPrescanning);
        
        if(externalLogs)
            env->closeLogStream();
        
        cout << "--- End of network pre-scanning (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&prescanningEnd, NULL);
        unsigned long prescanningElapsed = prescanningEnd.tv_sec - prescanningStart.tv_sec;
        double successRate = ((double) env->getTotalSuccessfulProbes() / (double) env->getTotalProbes()) * 100;
        size_t nbResponsiveIPs = env->getIPTable()->getTotalIPs();
        cout << "Elapsed time: " << elapsedTimeStr(prescanningElapsed) << endl;
        cout << "Total amount of probes: " << env->getTotalProbes() << endl;
        cout << "Total amount of successful probes: " << env->getTotalSuccessfulProbes();
        cout << " (" << successRate << "%)" << endl;
        cout << "Total amount of discovered responsive IPs: " << nbResponsiveIPs << "\n" << endl;
        env->resetProbeAmounts();

        delete prescanner;
        prescanner = NULL;
        
        if(nbResponsiveIPs == 0)
        {
            cout << "Could not discover any responsive IP. Program will halt now." << endl;
            delete parser;
            delete env;
            return 0;
        }
        
        // Gets targets for scanning
        list<InetAddress> targets = parser->getTargetsScanning();
        
        // Parser is no longer needed
        delete parser;
        parser = NULL;
        
        /*
         * STEP II: NETWORK SCANNING
         *
         * Given the set of (responsive) target addresses, SAGE starts scanning the network by 
         * launching subnet discovery threads on each target. The inferred subnets are later 
         * merged together (when it is possible) to obtain a clean set of subnets where no subnet 
         * possibly contain another entry in the set.
         *
         * Note that this phase also includes refinement steps, both during the scanning itself 
         * (known as "bypass", see NetworkScanner class) and after the completion of scanning.
         */
        
        cout << "--- Start of network scanning (" << getCurrentTimeStr() << ") ---" << endl;
        timeval scanningStart, scanningEnd;
        gettimeofday(&scanningStart, NULL);
        
        if(externalLogs)
            env->openLogStream("Log_" + newFileName + "_scanning");
       
        scanner = new NetworkScanner(env);
        scanner->scan(targets);
        scanner->finalize();
        
        if(externalLogs)
            env->closeLogStream();
        
        cout << "--- End of network scanning (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&scanningEnd, NULL);
        unsigned long scanningElapsed = scanningEnd.tv_sec - scanningStart.tv_sec;
        successRate = ((double) env->getTotalSuccessfulProbes() / (double) env->getTotalProbes()) * 100;
        size_t nbDiscoveredSubnets = subnetSet->getSubnetSiteList()->size();
        cout << "Elapsed time: " << elapsedTimeStr(scanningElapsed) << endl;
        cout << "Total amount of probes: " << env->getTotalProbes() << endl;
        cout << "Total amount of successful probes: " << env->getTotalSuccessfulProbes();
        cout << " (" << successRate << "%)" << endl;
        cout << "Total amount of discovered subnets: " << nbDiscoveredSubnets << "\n" << endl;
        env->resetProbeAmounts();
        
        delete scanner;
        scanner = NULL;
        
        if(nbDiscoveredSubnets == 0)
        {
            cout << "Could not discover any subnet. Program will halt now." << endl;
            delete env;
            return 0;
        }
        
        // Saves subnet inference details if user asked them.
        if(saveXNETRecords)
        {
            env->outputExploreNETRecords(newFileName + ".xnet");
            cout << "Details about subnet inference as carried out by ExploreNET have been ";
            cout << "written in a new file " << newFileName << ".xnet." << endl;
        }
        
        // 1st save of the inferred subnets (no route)
        subnetSet->outputAsFile(newFileName + ".subnets");
        cout << "Inferred subnets have been saved in an output file ";
        cout << newFileName << ".subnets.\n" << endl;
        outputtedSubnets = true;
        
        /*
         * STEP III: TRACEROUTE
         *
         * In order to locate subnets with respect to each other, SAGE conducts (Paris) traceroute 
         * measurements towards each measured subnets as well as a short "repair" phase which aims 
         * at mitigating incomplete steps found in routes as much as possible. The whole task is 
         * carried out by the SubnetTracer class, found in the traceroute module 
         * (see src/algo/traceroute/).
         */
        
        cout << "--- Start of traceroute ---" << endl;
        timeval tracerouteStart, tracerouteEnd;
        gettimeofday(&tracerouteStart, NULL);
        
        if(externalLogs)
            env->openLogStream("Log_" + newFileName + "_traceroute");
        
        tracer = new SubnetTracer(env);
        tracer->measure();
        tracer->repair(); // Not in measure(), because repairment could be made optional later
        
        delete tracer;
        tracer = NULL;
        
        if(externalLogs)
            env->closeLogStream();
        
        cout << "--- End of traceroute (" << getCurrentTimeStr() << ") ---" << endl;
        gettimeofday(&tracerouteEnd, NULL);
        unsigned long tracerouteElapsed = tracerouteEnd.tv_sec - tracerouteStart.tv_sec;
        successRate = ((double) env->getTotalSuccessfulProbes() / (double) env->getTotalProbes()) * 100;
        cout << "Elapsed time: " << elapsedTimeStr(tracerouteElapsed) << endl;
        cout << "Total amount of probes: " << env->getTotalProbes() << endl;
        cout << "Total amount of successful probes: " << env->getTotalSuccessfulProbes();
        cout << " (" << successRate << "%)\n" << endl;
        env->resetProbeAmounts();
        
        // Final save of the subnets, overriding previous save.
        subnetSet->outputAsFile(newFileName + ".subnets");
        cout << "Inferred subnets have been saved a second time (with traceroute measurements) ";
        cout << "in " << newFileName << ".subnets.\n" << endl;
        
        /*
         * STEP IV: GRAPH BUILDING
         *
         * After step II and step III, SAGE has all the important data to start building the graph 
         * modeling the target network. It first starts by deriving the neighborhood label of each 
         * subnet from their route (the neighborhood label being either the last IP on the route, 
         * either a sequence of one non-anonymous IP followed by excusively anonymous IPs), then 
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
            successRate = ((double) env->getTotalSuccessfulProbes() / (double) env->getTotalProbes()) * 100;
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
        
        /*
         * STEP V: ALIAS RESOLUTION
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
        successRate = ((double) env->getTotalSuccessfulProbes() / (double) env->getTotalProbes()) * 100;
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
        if(subnetSet->getSubnetSiteList()->size() > 0 || !dict->isEmpty())
            cout << endl;
    
        // Emergency save of subnets
        if(subnetSet->getSubnetSiteList()->size() > 0 && !outputtedSubnets)
        {
            subnetSet->outputAsFile("[Stopped] " + newFileName + ".subnets");
            cout << "Inferred subnets have been saved in a file \"[Stopped] "+ newFileName + ".subnet\"." << endl;
        }
        
        // Emergency save of the IP dictionnary
        if(!dict->isEmpty())
        {
            dict->outputDictionnary("[Stopped] " + newFileName + ".ips");
            cout << "IP dictionnary has been saved in a file \"[Stopped] "+ newFileName + ".ips\"." << endl;
        }
        
        // Because pointers are set to NULL after deletion, next lines should not cause any issue.
        delete parser;
        delete prescanner;
        delete scanner;
        delete tracer;
        delete builder;
        delete galileo;
        
        delete env;
        return 1;
    }
    
    delete env;
    return 0;
}
