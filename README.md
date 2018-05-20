# SAGE (Subnet AGgrEgation)

*By Jean-François Grailet (last updated: May 20, 2018)*

## Overview

`SAGE` is a topology discovery tool which aims at discovering an accurate picture of a target network by building a graph of its *neighborhoods*, i.e., network locations bordered by subnets located at at most one hop away from each other and which usually consist of one router or a mesh of routers (to be discovered with alias resolution). To achieve this goal, `SAGE` scans for subnets in the target domain and collects (Paris) `traceroute` measurements for each discovered subnet. Then, it runs various algorithms to aggregate the subnets and see how these aggregates are positioned with respect to each other to build a complete graph of the target network, using a minimal amount of information from each route.

In the process, `SAGE` uses active probing in order to perform alias resolution on router interfaces discovered by `traceroute` when it discovers these interfaces are topologically close to each other. By only using a small part of the collected routes and performing alias resolution to discover routing convergence points, `SAGE` avoids many of the issues one can encounter while interpreting `traceroute` data. `SAGE` is also able to infer which subnet implements a point-to-point link between two neighborhoods that are one hop away from each other, and therefore provides a very 
complete picture of the topology of a network.

To discover subnets, `SAGE` re-uses state-of-the-art algorithms from both `TreeNET` and `ExploreNET` and re-uses as well the alias resolution methodology of `TreeNET` to discover routers among the neighborhoods found in the final graph. As the subnet inference algorithms used in `SAGE` are designed for IPv4, `SAGE` is currently only available for IPv4 as a 32-bit application written in C/C++.

## Content of this repository

**N.B.: this repository is still under construction.**

This repository consists of the following content:

* **Dataset/** provides complete datasets for various Autonomous Systems (or ASes) we measured with `SAGE` from the PlanetLab testbed. Additionnal sub-folders provide Python scripts to have a better look at the data and validate a specific measurement methodology we used to measure large and/or highly responsive ASes.

* **Motivations/** provides some datasets collected with `TreeNET` in September 2017 along some Python scripts used to investigate the early intuitions behind the design of `SAGE`.

* **v1/** provides all the source files of `SAGE` v1.0, along some instructions to build and use it.

## Disclaimer

`SAGE` was written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN). Some parts of `SAGE` re-uses code from `TreeNET`, which itself re-uses code from `ExploreNET`. For more details on `TreeNET` and how it relates to previous software, check the `TreeNET` public repository:

https://github.com/JefGrailet/treenet

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `SAGE`. I am also inclined to answer questions regarding the algorithms used in `SAGE` and to discuss its application in other research projects.
