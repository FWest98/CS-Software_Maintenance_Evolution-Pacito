#!/bin/bash
#SBATCH --time=10-00:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --job-name=complete_cycle_pinot_mina
#SBATCH --mem=100GB
#SBATCH --mail-type=ALL
#SBATCH --mail-user=f.a.de.capela@student.rug.nl
#SBATCH --output=job-%complete_cycle_pinot_mina.log
#SBATCH --partition=regular

module load Java/1.7.0_80
export CLASSPATH=${CLASSPATH}:/apps/generic/software/Java/1.7.0_80/jre/lib/rt.jar

COUNTER=1

cd /data/s4040112/sourcecodes/mina
git pull
FINAL_COMMIT=$(git show -s --format=%H)

#depending on the project, the main branch can either be master or trunk
#git rev-list --reverse master > commitOrder.txt
git rev-list --reverse trunk > commitOrder.txt

filename=commitOrder.txt
file_lines=`cat $filename`

#mkdir -p outputs

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

for line in $file_lines ; 
do
	git reset --hard $line
    	CURRENT_COMMIT=$(git log -n1 --format=format:"%H")
	#sh /home/p289550/tools/Pinot/HPC-pinotscript.sh 2>&1 | tee /data/p289550/Pinot_results/Mina_results/$COUNTER-ID-$CURRENT_COMMIT.txt

	#don't forget to run sudo updatedb, since locate finds all files but needs to be updated using this command
	#updatedb

	find ${projectpath} -name '*.java' > ${projectname}-files.list
	#locate ${projectpath}**.java > ${projectname}-files.list

	if [ "$verbose" = true ] ; then
	echo "$(<${projectname}-files.list)"
	fi

	#java -jar /data/s4040112/Internship_RuG_2020/0-ProjectRefactorer/out/artifacts/0_ProjectRefactorer_jar/0-ProjectRefactorer.jar $projectname

	/home/s4040112/tools/bin/pinot @${projectname}-files.list 2>&1 | tee /data/s4040112/Pinot_results/outputs-${projectname}/$COUNTER-ID-$CURRENT_COMMIT.txt

	COUNTER=$((COUNTER+1))
	git log -1 --pretty=format:"%h - %an, %ar"
	echo $(git log $CURRENT_COMMIT..$FINAL_COMMIT --pretty=oneline | wc -l) " - Number of commits left"
done

cd /data/s4040112

#create directory to store one complete analysis
mkdir -p ${projectname}-completeCycle

#mv the outputs from pinot to the specified folder
mv /data/s4040112/Pinot_results/outputs-${projectname}/ ${projectname}-completeCycle/

cd /data/s4040112/${projectname}-completeCycle/outputs-${projectname}/

cp /data/s4040112/Internship_RuG_2020/Scripts_and_Tools/HPC-Scripts/HPC-blank-error-validChecker.sh .

#Create 4 files with information regarding empty, blank or valid files
./HPC-blank-error-validChecker.sh ${projectname}

mkdir -p additionalInformation

mv ${projectname}-finalAnalysis.txt additionalInformation

#Create csv with progress of analysis over time (error, blank, valid)
java -jar /data/s4040112/Internship_RuG_2020/1-pinotAnalysisProgressChecker/out/artifacts/pinotAnalysisProgressChecker_jar/pinotAnalysisProgressChecker.jar $projectname

mv ${projectname}-finalAnalysis.csv additionalInformation

cd /data/s4040112/${projectname}-completeCycle

#Scan consecutive commits for comparison of patterns
java -jar /data/s4040112/Internship_RuG_2020/2-PinotOutputComparator/out/artifacts/PinotOutputComparator_jar/PinotOutputComparator.jar outputs-${projectname}

cd results-${projectname}

cp /data/s4040112/Internship_RuG_2020/Scripts_and_Tools/HPC-Scripts/HPC-issueTagExtractor.sh .

./HPC-issueTagExtractor.sh ${projectname}

mv ${projectname}-issueTags/ ../

#Run java project to obtain information from JIRA's issue by using the XML information online
java -jar /data/s4040112/Internship_RuG_2020/3-JiraIssueParser/out/artifacts/JiraIssueParser_jar/JiraIssueParser.jar ${projectname}-issueTags







