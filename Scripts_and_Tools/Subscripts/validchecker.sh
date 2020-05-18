#!/bin/bash
 
if [ $# -eq 0 ]
  then
    echo "No arguments supplied. Usage ./validchecker.sh desiredFileName"
fi

touch $1-valid.list

#Search for empty files, since these also indicate that a problem happened amist pinot's execution
egrep -l "Pattern" *.txt >> $1-valid.list

#Since linux sorts alphabetically by default, a sort needs to be made for better reading of the
#commit-id's that failed 
sort -n -o $1-valid.list $1-valid.list
