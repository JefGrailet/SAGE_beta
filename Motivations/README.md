# SAGE motivations

*By Jean-François Grailet (last updated: May 20, 2018)*

This repository contains subnet data collected with `TreeNET` on September 1st, 2017 along some Python scripts we used to investigate the early ideas of `SAGE` but also evaluate the extent of `traceroute` issues which makes the interpration of `TreeNET` tree-like view of a network more difficult.

# About the scripts

Two Python scripts are provided here:

* **PeersDistance.py** takes a look at the routes towards each subnet in our whole dataset to make a census of their respective last hop (i.e., last hop before the subnet; therefore the penultimate IP if we take the full route with the target IP) and check whether the previous hops in the route are also last hops in other routes (this is actually a simplified definition of a *peer* in the terminology of `SAGE`). Then, the script evaluates the typical distance between such IPs. The script plots the result as a PDF (Probability Density Function), where the X axis pictures the difference of distance in terms of hop(s)/TTL. Note that the PDF curve does not reach exactly 100%, because in some routes, there is no hop before the last hop that appears to be a last hop in another route.

* **TracerouteIllnesses.py** also takes a look at the routes towards each subnet of the dataset, but rather evaluates the extent of some typical `traceroute` issues that are a problem for `TreeNET`. In particular, it quantifies the ratio of routes which feature anonymous hop(s), the ratio of routes which ends in an anonymous hop and the ratio of routes where the last hop before the subnet is *stretched*, i.e., it was observed at different distances TTL-wise within the same dataset. The script plots the result as bar charts, with 3 bars for each AS of the dataset.

To run these scripts with the available data, you can just run these commands:

```
python PeersDistance.py 2017 01-09 ASesList
python TracerouteIllnesses.py 2017 01-09 ASesList
```

where *ASesList* is a text file listing the measured ASes along their type.

# About the subnet data

In this repository, we only provide the *.subnets* file obtained for each of the 10 following ASes on September 1st, 2017:

|   AS    | AS name                  | Type    | Max. amount of IPs |
| :-----: | :----------------------- | :------ | :----------------- |
| AS14    | University of Columbia   | Stub    | 339,968            |
| AS109   | Cisco Systems            | Stub    | 1,173,760          |
| AS224   | UNINETT                  | Stub    | 1,115,392          |
| AS5400  | British Telecom          | Transit | 1,385,216          |
| AS5511  | Orange S.A.              | Transit | 911,872            |
| AS6453  | TATA Communications      | Tier-1  | 656,640            |
| AS703   | Verizon Business         | Transit | 863,232            |
| AS8928  | Interoute Communications | Transit | 827,904            |
| AS12956 | Telefonica International | Tier-1  | 209,920            |
| AS13789 | Internap Network         | Transit | 96,256             |

Note that the amount of IPs in the table were the correct amounts at the time of the measurements. They might have changed since then.

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
