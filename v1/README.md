# About SAGE v1.0

*By Jean-François Grailet (last updated: August 28, 2018)*

## Content of this repository

`SAGE` v1.0 comes in two different versions.

* **Discovery** is the main implementation of `SAGE` and the version you should go after if you just want to use `SAGE` on small/medium topologies. It performs all at once network scanning (i.e., discovering subnets), subnet tracing (i.e., discovering routes towards each subnet), graph building and alias resolution (on the neighborhoods in the final graph).

* **Fusion** is a special version of `SAGE` which just implements the graph building and alias resolution phases. Indeed, the purpose of this version is to merge together several datasets into one and build a single graph with it. Since `SAGE` only uses the last relevant interfaces on the routes towards each subnet, it is possible to build neighborhoods and locate them with respects to each other while having routes collected from several vantage points. `SAGE` "*Fusion*" should be used only if you intend to measure large networks for which a single instance of `SAGE` will be unable to complete its execution in a reasonable amount of time.

## Disclaimer

`SAGE` was written by Jean-François Grailet, currently Ph. D. student at the University of Liège (Belgium) in the Research Unit in Networking (RUN). Some parts of `SAGE` re-uses code from `TreeNET`, which itself re-uses code from `ExploreNET`. For more details on `TreeNET` and how it relates to previous software, check the check the [`TreeNET` public repository](https://github.com/JefGrailet/treenet).

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/

Feel free to send me an e-mail if your encounter problems while compiling or running `SAGE`. I am also inclined to answer questions regarding the algorithms used in `SAGE` and to discuss its application in other research projects.
