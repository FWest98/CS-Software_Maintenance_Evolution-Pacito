#!/bin/bash

projectname="mina"

arr=()

touch ${projectname}-files-temp.list

for 
while read line
  do
	NAMEWITHOUTEXTENSION=$(echo $line | cut -d'.' -f 1)
	touch $NAMEWITHOUTEXTENSION-temp.java 

	while read fileline
  		do
			if [[ $fileline == *"<"* ]]; then
				INSIDEDIAMONDS=$(echo $fileline | awk -F[\<\>] '{print $2}')
				arr+=($INSIDEDIAMONDS)
    				echo "$line - Class was processed, good result!"
			else
				if [ "$fileline" == "Number of classes processed: 0" ]; then
    					echo "$line - No class was processed."	
				fi

			fi
  	done < $line
 done < ${projectname}-files.list





FOR ALL FILES
	CREATE COPY FILE,
	ADD FILENAME TO PROJECTNAME-TEMPFILES.LIST, 
	CHECK IF LINE CONTAINS <> OR CONTAINS SOMETHING FOUND INSIDE THE DIAMOND BEFORE AND REPLACE WITH OBJECT,
	IF IT DOES, CHANGE CONTENT AND WRITE TO NEWFILE,
	OTHERWISE JUST WRITE THE LINE TO NEWFILE,
END

DELETE PROJECTNAME-FILES.LIST,
RENAME PROJECTNAME-TEMPFILES.LIST TO PROJECTNAME-FILES.LIST,

