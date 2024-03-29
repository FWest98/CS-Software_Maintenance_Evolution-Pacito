#!/bin/bash

COUNTER=1

git pull
FINAL_COMMIT=$(git show -s --format=%H)

#depending on the project, the main branch can either be master or trunk
git rev-list --reverse master > commitOrder.txt
#git rev-list --reverse trunk > commitOrder.txt

filename=commitOrder.txt
file_lines=`cat $filename`

mkdir -p outputs

for line in $file_lines ; 
do
	git reset --hard $line
    	CURRENT_COMMIT=$(git log -n1 --format=format:"%H")
	./pinotscript.sh 2>&1 | tee outputs/$COUNTER-ID-$CURRENT_COMMIT.txt
	COUNTER=$((COUNTER+1))
	git log -1 --pretty=format:"%h - %an, %ar"
	echo $(git log $CURRENT_COMMIT..$FINAL_COMMIT --pretty=oneline | wc -l) " - Number of commits left"
done
