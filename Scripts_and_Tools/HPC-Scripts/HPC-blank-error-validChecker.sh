#!/bin/bash
 
if [ $# -eq 0 ]
  then
    echo "No arguments supplied. Usage ./blank-error-validChecker.sh desiredFileName"
    exit
fi

touch $1-blanks.list
touch $1-errors.list
touch $1-valid.list

#Search for empty files, since these also indicate that a problem happened amist pinot's execution
egrep -L " " *.txt >> $1-blanks.list

#Since linux sorts alphabetically by default, a sort needs to be made for better reading of the
#commit-id's that failed 
sort -n -o $1-blanks.list $1-blanks.list

#Search for every occurence of either pinot or jikes in the output, which indicates errors
#during pinots execution, and redirecting its output to a file
egrep -l -e pinot -e jikes *.txt >> $1-errors.list

#Since linux sorts alphabetically by default, a sort needs to be made for better reading of the
#commit-id's that failed 
sort -n -o $1-errors.list $1-errors.list

#Search for empty files, since these also indicate that a problem happened amist pinot's execution
egrep -l "Pattern" *.txt >> $1-valid.list

#Since linux sorts alphabetically by default, a sort needs to be made for better reading of the
#commit-id's that failed 
sort -n -o $1-valid.list $1-valid.list

touch $1-finalAnalysis.txt

wc -l $1-blanks.list >> $1-finalAnalysis.txt
wc -l $1-errors.list >> $1-finalAnalysis.txt
wc -l $1-valid.list >> $1-finalAnalysis.txt
