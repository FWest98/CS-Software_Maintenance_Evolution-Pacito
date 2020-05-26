#!/bin/bash

COUNTER=1
mkdir -p outputs-individual-files

while read line
  do
	CURRENTNAME=$(echo $line | rev | cut -d'/' -f-3,-2,-1 | rev | tr '/' -)
     	pinot $line 2>&1 | tee outputs-individual-files/${COUNTER}_${CURRENTNAME}.txt
      	COUNTER=$((COUNTER+1))
  done < mina-files.list
