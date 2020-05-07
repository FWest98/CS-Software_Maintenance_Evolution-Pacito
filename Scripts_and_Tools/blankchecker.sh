#!/bin/bash
 
if [ $# -eq 0 ]
  then
    echo "No arguments supplied. Usage ./outputchecker desiredFileName"
fi

touch $1-blanks.list

#Search for empty files, since these also indicate that a problem happeneded amist pinot's execution
egrep -L " " *.txt >> $1-blanks.list

#Since linux sorts alphabetically by default, a sort needs to be made for better reading of the
#commit-id's that failed 
sort -n -o $1-blanks.list $1-blanks.list
