#!/bin/bash

datasets=./

n_ASes=6

ASes[1]="AS109"
ASes[2]="AS224"
ASes[3]="AS5400"
ASes[4]="AS6453"
ASes[5]="AS8928"
ASes[6]="AS12956"

dateComp[1]="03-04"
dateComp[2]="05-04"
dateComp[3]="03-04"
dateComp[4]="05-04"
dateComp[5]="05-04"
dateComp[6]="05-04"

dateParts[1]="01-04"
dateParts[2]="04-04"
dateParts[3]="01-04"
dateParts[4]="04-04"
dateParts[5]="04-04"
dateParts[6]="04-04"

i=1
while [ $i -le $n_ASes ]
do
    echo "${ASes[$i]}"
    firstGraphPath=$datasets/${ASes[$i]}/2018/${dateComp[$i]}/${ASes[$i]}\_${dateComp[$i]}.graph
    secondGraphPath=$datasets/${ASes[$i]}/2018/${dateParts[$i]}/${ASes[$i]}\_${dateParts[$i]}.graph
    python GraphComparison.py $firstGraphPath $secondGraphPath
    i=`expr $i + 1`
done
