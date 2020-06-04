#!/bin/bash


#This script is supposed to be ran inside a folder containing outputs from the Java project, more specifically, folders inside the folder numbered with 2 inside the Outputs folder

if [ $# -eq 0 ]
  then
    echo "No arguments supplied. Usage ./issueTagExtractor.sh gitHubFolderName\nThis gitHubFolderName needs to exactly match the one on your system"
fi

touch commitHashes.list

egrep -l commit VALID-* > commitHashes.list

currentFolder=$(pwd)

mkdir -p $1-issueTags

cp [vV]* ./$1-issueTags

input="commitHashes.list"
while IFS= read -r line
do
    FIRSTLINE=`head -n 1 $line`
    prefix="Pattern changes caused by commit: "
    commitHash=${FIRSTLINE#$prefix} 

    suffix=".txt"
    filenameWithoutTxt=${line%$suffix}

    cd /data/s4040112/sourcecodes/$1

    echo -e "\n=========================\n       NEW GIT LOG\n=========================\n" > $filenameWithoutTxt-issueTag.txt

    echo -e "This commit refers to file: $line \n" >> $filenameWithoutTxt-issueTag.txt

    git log --pretty=short $commitHash -n 1 >> $filenameWithoutTxt-issueTag.txt
 
    cat $filenameWithoutTxt-issueTag.txt >> $currentFolder/$1-issueTags/$line

    rm $filenameWithoutTxt-issueTag.txt

    cd $currentFolder
done < "$input"


