#!/bin/bash

#if [ -d ~/Downloads/ResearchProject/Internship_RuG_2020/Analyzed_Projects/outputs ] 
#then
#    echo "Directory /outputs exists." 
#else
#    sudo mkdir ~/Downloads/ResearchProject/Internship_RuG_2020/Analyzed_Projects/outputs
#fi

#sudo chmod 777 ~/Downloads/ResearchProject/Internship_RuG_2020/Analyzed_Projects/outputs

COMMIT_ID=$(git rev-parse --verify HEAD)

NUMBER_OF_COMMITS=$(git rev-list --count $COMMIT_ID)

COUNTER=1

while [ $NUMBER_OF_COMMITS -gt 0 ]
do
	COMMIT_ID2=$(git rev-parse --verify HEAD)
	./pinotscript.sh | sudo tee ~/Downloads/ResearchProject/Internship_RuG_2020/Analyzed_Projects/$1/$COUNTER-ID:$COMMIT_ID2.txt
	git reset --hard HEAD~1 
	NUMBER_OF_COMMITS2=$(git rev-list --count $COMMIT_ID2)
	COUNTER=$((COUNTER+1))
done
