# Measuring an AS with multiple VPs: analysis

*By Jean-François Grailet (last updated: May 23, 2018)*

## About this folder

This folder contains the data used to compare the measurement of an AS with a single PlanetLab 
node and the same measurement using multiple PlanetLab nodes, in order to show that both 
approaches produce a similar graph despite that the second approach involves different vantage 
points from which the `traceroute` data will be very different.

The composition of this sub-folder is the same as the *Dataset/* folder, except it only contains
the datasets that are being compared.

To compare them, you can simply run this command:

```sh
./Comparison.sh
```

This will automatically run the Python script *GraphComparison.py* multiple times with the right 
arguments.

## Contact

**E-mail address:** Jean-Francois.Grailet@uliege.be

**Personal website:** http://www.run.montefiore.ulg.ac.be/~grailet/
