#!/bin/bash
 
if [ $# -eq 0 ]
  then
    echo "No arguments supplied. Usage ./errorchecker.sh desiredFileName"
fi

touch $1-errors.list

#Search for every occurence of either pinot or jikes in the output, which indicates errors
#during pinots execution, and redirecting its output to a file
egrep -l -e pinot -e jikes *.txt >> $1-errors.list

#Since linux sorts alphabetically by default, a sort needs to be made for better reading of the
#commit-id's that failed 
sort -n -o $1-errors.list $1-errors.list
