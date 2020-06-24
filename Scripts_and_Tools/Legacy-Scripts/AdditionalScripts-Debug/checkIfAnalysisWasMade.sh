#!/bin/bash

find . -name '*.txt' > /outputs-individual-files/individual-files.list

while read line
  do
	while read fileline
  		do
			if [ "$fileline" == "Number of classes processed: 1" ]; then
    				echo "$line - Class was processed, good result!"
			else
				if [ "$fileline" == "Number of classes processed: 0" ]; then
    					echo "$line - No class was processed."	
				fi
			fi
  	done < $line
  done < /outputs-individual-files/individual-files.list
