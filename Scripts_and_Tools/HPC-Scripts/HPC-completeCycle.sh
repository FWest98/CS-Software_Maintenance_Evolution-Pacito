#!/bin/bash
#SBATCH --time=10-00:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --job-name=PACITO_Apache_Mina
#SBATCH --mem=100GB
#SBATCH --mail-type=ALL
#SBATCH --mail-user=f.a.de.capela@student.rug.nl
#SBATCH --output=job-%j-PACITO_Apache_Mina.log
#SBATCH --partition=regular

module load Maven/3.5.2
module load Java/1.7.0_80
export CLASSPATH=${CLASSPATH}:/apps/generic/software/Java/1.7.0_80/jre/lib/rt.jar

COUNTER=1

#This value needs to be changed according to the number of issues the analyzed software contains in 
#the JIRA repository, for more information check the comments in the Java project 3-JiraIssueParser
NUMBEROFISSUES=1126

cd /data/s4040112/sourcecodes/mina
git pull

#A new .mailmap needs to be created for different projects, hence, change the content of the file or change
#the location to match the location of the new .mailmap
cp /data/s4040112/Internship_RuG_2020/Scripts_and_Tools/HPC-Scripts/.mailmap .
git shortlog -se -n > listOfContributionsPerDeveloper.txt

FINAL_COMMIT=$(git show -s --format=%H)

#depending on the project, the main branch can either be master or trunk
#git rev-list --reverse master > commitOrder.txt
git rev-list --reverse trunk > commitOrder.txt

filename=commitOrder.txt
file_lines=`cat $filename`

projectpath="/data/s4040112/sourcecodes/mina"
projectname="mina"
verbose=false
TEMP=`getopt --long -o "p:v" "$@"`
eval set -- "$TEMP"
while true ; do
case "$1" in
-p )
projectpath=$2
projectnametmp=${projectpath%/}
projectname=${projectnametmp##*/}
unset projectnametmp
shift 2;;
-v )
verbose=true
shift ;;
--) shift ; break ;;
*)
break;;
esac
done

mkdir /data/s4040112/pinot_outputs-${projectname}

for line in $file_lines ; 
do
	git reset --hard $line
  CURRENT_COMMIT=$(git log -n1 --format=format:"%H")

	find ${projectpath} -name '*.java' > ${projectname}-files.list

	if [ "$verbose" = true ] ; then
	echo "$(<${projectname}-files.list)"
	fi
 
  sed -i '/AcceptorTest/d' ${projectname}-files.list
  sed -i '/ByteBufferProxy/d' ${projectname}-files.list
  sed -i '/HttpRequestEncoder/d' ${projectname}-files.list
  
  java -jar /data/s4040112/Internship_RuG_2020/0-ProjectRefactorer/out/artifacts/0_ProjectRefactorer_jar/0-ProjectRefactorer.jar $projectname

  FILE=pom.xml
  if test -f "$FILE"; then
  
    export CLASSPATH=
    export CLASSPATH=${CLASSPATH}:/apps/generic/software/Java/1.7.0_80/jre/lib/rt.jar

    find -name "pom.xml" > ${projectname}-poms.list
    
    java -jar /data/s4040112/Internship_RuG_2020/4-PomFileManipulator/out/artifacts/4_PomFileManipulator_jar/4-PomFileManipulator.jar ${projectname}
    
    mvn dependency:copy-dependencies -DoutputDirectory=/data/s4040112/sourcecodes/${projectname}/dependencies -Dhttps.protocols=TLSv1.2
  
 	  find ${projectpath} -name '*.jar' > ${projectname}-jars.list
 
	  while read line
    do
 	   export CLASSPATH=${CLASSPATH}:$line
    done < ${projectname}-jars.list
 
    rm ${projectname}-jars.list
 
  fi
  
  echo "$line"
  
  #Change depending on the version of pinot needed, the version inside tools2 does not scan the Factory Patterns due to its instability issues
  #/home/s4040112/tools/bin/pinot @${projectname}-newfiles.list 2>&1 | tee /data/s4040112/pinot_outputs-${projectname}/$COUNTER-ID-$CURRENT_COMMIT.txt
  /home/s4040112/tools2/bin/pinot @${projectname}-newfiles.list 2>&1 | tee /data/s4040112/pinot_outputs-${projectname}/$COUNTER-ID-$CURRENT_COMMIT.txt

  rm -rf dependencies

	rm ${projectname}-files.list
 
	rm ${projectname}-newfiles.list

	find ${projectpath} -name '*refactored.java' > ${projectname}-deletefiles.list

	while read line
	  do
		rm $line
 	done < ${projectname}-deletefiles.list
  
  rm ${projectname}-deletefiles.list
  
  rm -rf dependencies

	COUNTER=$((COUNTER+1))
	git log -1 --pretty=format:"%h - %an, %ar"
	echo $(git log $CURRENT_COMMIT..$FINAL_COMMIT --pretty=oneline | wc -l) " - Number of commits left"
done

module load Java/1.8.0_192
export CLASSPATH=${CLASSPATH}:/apps/generic/software/Java/1.8.0_192/jre/lib/rt.jar

cd /data/s4040112

#create directory to store one complete analysis
mkdir -p ${projectname}-completeCycle

#mv the outputs from pinot to the specified folder
mv /data/s4040112/pinot_outputs-${projectname}/ ${projectname}-completeCycle/

cd /data/s4040112/${projectname}-completeCycle/pinot_outputs-${projectname}

cp /data/s4040112/Internship_RuG_2020/Scripts_and_Tools/HPC-Scripts/HPC-blank-error-validChecker.sh .

chmod +rwx HPC-blank-error-validChecker.sh

#Create 4 files with information regarding empty, blank or valid files
./HPC-blank-error-validChecker.sh ${projectname}

rm -rf HPC-blank-error-validChecker.sh

mkdir -p additionalInformation

mv ${projectname}-finalAnalysis.txt ${projectname}-valid.list ${projectname}-blanks.list ${projectname}-errors.list additionalInformation

cd additionalInformation

#Create csv with progress of analysis over time (error, blank, valid)
java -jar /data/s4040112/Internship_RuG_2020/1-pinotAnalysisProgressChecker/out/artifacts/pinotAnalysisProgressChecker_jar/pinotAnalysisProgressChecker.jar $projectname

cd ..

mv additionalInformation ..

cd /data/s4040112/${projectname}-completeCycle

#Scan consecutive commits for comparison of patterns
java -jar /data/s4040112/Internship_RuG_2020/2-PinotOutputComparator/out/artifacts/PinotOutputComparator_jar/PinotOutputComparator.jar pinot_outputs-${projectname}

cd comparison_results-${projectname}

cp /data/s4040112/Internship_RuG_2020/Scripts_and_Tools/HPC-Scripts/HPC-issueTagExtractor.sh .

chmod +rwx HPC-issueTagExtractor.sh

./HPC-issueTagExtractor.sh ${projectname}

rm -rf HPC-issueTagExtractor.sh

mv ${projectname}-issueTags/ ..

cd ..

#Run java project to obtain information from JIRA's issue by using the XML information online
java -jar /data/s4040112/Internship_RuG_2020/3-JiraIssueParser/out/artifacts/JiraIssueParser_jar/JiraIssueParser.jar ${projectname}-issueTags $NUMBEROFISSUES

echo "Complete Cycle finished!"