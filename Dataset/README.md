# Dataset

*By Jean-François Grailet (last updated: May 23, 2018)*

## Measured Autonomous Systems (ASes)

The following ASes were measured on a regular basis (i.e., every 2 or 3 days).

|   AS    | AS name                  | Type    | Max. amount of IPs |
| :-----: | :----------------------- | :------ | :----------------- |
| AS109   | Cisco Systems, Inc.      | Stub    | 1,165,568          |
| AS12956 | Telefonica Int.          | Tier-1  | 191,488            |
| AS224   | UNINETT                  | Stub    | 1,114,880          |
| AS286   | KPN B. V.                | Tier-1  | 251,392            |
| AS5400  | British Telecom plc      | Transit | 925,696            |
| AS6453  | TATA COMMUNICATIONS      | Tier-1  | 559,360            |
| AS6762  | TELECOM ITALIA Sp. A.    | Tier-1  | 137,216            |
| AS6939  | Hurricane Electric       | Transit | 498,432            |
| AS8928  | Interoute Com.           | Transit | 886,528            |

The following ASes were measured once or twice, using several PlanetLab nodes.

|   AS    | AS name                  | Type    | Max. amount of IPs |
| :-----: | :----------------------- | :------ | :----------------- |
| AS10010 | TOKAI Communications     | Transit | 1,455,104          |
| AS1136  | KPN B. V.                | Transit | 6,704,896          |
| AS1239  | Sprint                   | Tier-1  | 18,197,248         |
| AS1241  | Forthnet                 | Transit | 677,888            |
| AS2497  | Internet Initiative Jp.  | Transit | 3,802,368          |
| AS3209  | Vodafone GmbH            | Transit | 8,450,048          |
| AS3257  | Tinet Spa                | Tier-1  | 1,010,176          |
| AS3549  | Level 3 Com. (GBLX)      | Transit | 6,708,992          |
| AS5432  | Proximus                 | Transit | 3,926,528          |
| AS5483  | Magyar Telekom Plc.      | Transit | 1,389,056          |
| AS8220  | COLT Technology          | Transit | 1,277,184          |
| AS8422  | NetCologne GmbH          | Transit | 559,616            |
| AS9198  | Kazakhtelecom            | Transit | 1,592,576          |

A few remarks about these lists:

* There is a bit less data for 3 ASes (AS286, AS6762 and AS6939) listed in the first list, because 
  they were added during the course of Spring 2018.

* The amount of IPs are based on the maximum capacity of the IPv4 prefixes listed for each AS. The 
  IPv4 prefixes were obtained via the BGP toolkit of Hurricane Electric in early Spring 2018. You 
  can access and use the BGP toolkit at the following address:
  
  http://bgp.he.net

* The AS classification (i.e., whether it's a stub, transit or tier-1 AS) is derived from a 2013 
  dataset provided by CAIDA which you can access here:
  
  http://data.caida.org/datasets/as-relationships/

* It's also worth noting we used the *AS Rank* dataset also provided by CAIDA in order to select
  some of the ASes we probed with `SAGE`. You can access this dataset at the following address:
  
  http://as-rank.caida.org/

## Composition of each dataset

For each AS, you will the list of IPv4 prefixes retrieved with the BGP toolkit from Hurricane 
Electric along with sub-folders matching the year and date of measurement. Each unique dataset
is matched with a sub-path /yyyy/dd-mm/. Then, for each dataset, you will find the following 
files:

* **.aliases file:** contains all the aliases discovered by `SAGE`, separated per neighborhood.
* **.fingerprints file:** contains all the fingerprints for each IP involved in the alias 
  resolution process, separated per neighborhood.
* **.graph file:** describes the graph built by `SAGE`.
* **.ips file:** lists all responsive IPs (including those found with `traceroute`) and their 
  associated data, i.e., IP dictionary.
* **.metrics file:** gives a variety of metrics on the data collected by `SAGE` and the structure 
  of the graph.
* **.neighborhoods file:** lists all neighborhoods discovered by `SAGE` along the subnets which 
  border each of them.
* **.subnets file:** lists, in order, all the subnets that were inferred along their responsive
  interfaces and respective route.
* **.txt file:** gives the details about how the measurement went, i.e., it gives the detailed 
  amount of probes used by each phase along the time they took for completion.
* **Log_\*_graph:** gives the detailed console output of the graph building phase. In particular,
  it describes the initial aggregates and their peers and gives all the details about alias
  resolution when it occurs.
* **VP.txt:** gives the PlanetLab node (the **v**antage **p**oint) used to measure the AS. If the 
  AS was measured using multiple nodes, then VP.txt gives the PlanetLab node used to merge the
  data and complete the last steps of `SAGE`.

If the dataset also contains a *Part/* sub-folder, then the AS was measured with multiple 
PlanetLab nodes and you will find inside it the individual *.subnets* files (for each part of the 
AS) along *VP_X.txt* files which give the PlanetLab node used to get each individual subnet set.

Note that the date of measurement is only the date at which the measurement was started. As the 
total time to completely discover an AS can exceed 24 hours, the date at which the measurement was 
completed can be different than the date at which the measurement began.

## Sub-folders

In addition to the sub-folder dedicated to each measured AS, you can also find these sub-folders:

* **MultipleNodes/** contains specific datasets used to compare the measurement of an AS with a 
  single PlanetLab node (whole list of prefixes) or multiple PlanetLab nodes (disjoint lists of 
  prefixes, each node having a single list), in order to confirm the key features of the 
  resulting graphs remain similar (as large ASes could only be measured by involving several 
  PlanetLab nodes in the measurement).

* **Scripts/** contains various Python scripts to analyze our datasets and plot figures describing 
  their key features (largest connected component, largest clusters, neighborhood degree 
  distribution, etc.).

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
