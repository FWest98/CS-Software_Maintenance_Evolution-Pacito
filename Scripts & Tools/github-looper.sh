#!/bin/bash

COMMIT_ID=$(git rev-parse --verify HEAD)

NUMBER_OF_COMMITS=$(git rev-list --count $COMMIT_ID)

while [ $NUMBER_OF_COMMITS -gt 0 ]
do
	./pinotscript.sh
	git reset --hard HEAD~1
	NUMBER_OF_COMMITS=$(git rev-list --count $COMMIT_ID)
	echo $NUMBER_OF_COMMITS
done
