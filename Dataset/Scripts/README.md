# Python scripts for data analysis

*By Jean-François Grailet (last updated: May 25, 2018)*

## About this folder

This folder contains a variety of Python scripts you can use to evaluate the target files of our
dataset as well as visualize the data collected by `SAGE` through various graphs highlighting the
most notable caracteristics of each AS.

For scripts which output figures, please note that each of them requires a *Dataset* text file 
which gives the ASes of interest, their classification (i.e. stub, transit or tier-1) and the date 
of the dataset/snapshot the script should use to compute its results.

Here is the detailed list of the scripts which are available here:

* **CountTargets.py:** use this script to see the amount of targets for each AS. If an AS is 
  measured with several vantage points, the script will give the amount of target IPs for each 
  individual set of targets (in addition to the total).

* **InDegreeClusters.py:** for a given *Dataset* file, evaluates the maximum and average entering 
  degree of clusters in each dataset and outputs the results as an annotated bar chart (annotation 
  gives the average entering degree).

* **InternetCoverage.py:** evaluates the ratio of IPs covered by our target files with respects to
  the whole IPv4 range.

* **LargestClusters.py:** for a given *Dataset* file, finds the largest clusters in each AS and 
  count the total of clusters in each. It outputs the results as an annotated bar chart 
  (annotation gives the total of clusters).

* **LargestComponents.py:** for a given *Dataset* file, gets the size of the largest connected 
  component in each graph and outputs the result as a bar chart.

* **LargestNeighborhoods.py:** for a given *Dataset* file, gets the size of the largest 
  neighborhood in each graph and outputs the result as a bar chart. The bars are annotated with 
  the amount of neighborhoods, in each AS, which gather 200 or more subnets.

* **NeighborhoodDegrees.py:** for a given *Dataset* file, re-builds the graphs for each AS an 
  adjacency matrix to evaluate the degree of all neighborhoods in the whole dataset and compute 
  the CDFs of the degree of neighborhoods for stub, transit and tier-1 AS. The 3 CDFs are 
  outputted in the same figure. **Warning:** building the adjacency matrixes takes time, so 
  executing this script for all ASes can take more than one minute.

* **SubnetsAsLinks.py:** for a given *Dataset* file, analyzes the links in each graph to see 
  what kind of subnet is matched with each link, and computes, for all listed ASes, the 
  distribution of the subnet prefix lengths. It also produces a second figure where the average
  amount of implemented links is given for each prefix length.

**Note:** for scripts outputting bar charts, ASes are denoted by an index (i.e., 1, 2, 3...) 
rather than the ASN, due to a lack of place. The mapping between these indexes and the ASes is 
given at the end of the execution of the script.

## Commands

If you want to test the scripts listed above right away (except for *CountTargets.py*, which 
requires an AS as argument), you can `cd` into this folder (after cloning the repository) and run 
these commands:

```sh
python InternetCoverage.py
python InDegreeClusters.py Dataset
python LargestClusters.py Dataset
python LargestComponents.py Dataset
python LargestNeighborhoods.py Dataset
python NeighborhoodDegrees.py Dataset
python SubnetsAsLinks.py Dataset
```

If you need to run (for whatever reason) these scripts outside this folder, be sure to change the
data paths in the scripts. They are signaled in the code by TODO's.

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
