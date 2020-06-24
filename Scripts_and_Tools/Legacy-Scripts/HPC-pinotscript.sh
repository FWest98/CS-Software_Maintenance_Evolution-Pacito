#!/bin/bash
#SBATCH --time=10-00:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --job-name=preprocessing
#SBATCH --mem=100GB
#SBATCH --mail-type=ALL
#SBATCH --mail-user=f.a.de.capela@student.rug.nl
#SBATCH --output=job-%j-preprocessing.log
#SBATCH --partition=regular

module load Java/1.7.0_80
export CLASSPATH=${CLASSPATH}:/apps/generic/software/Java/1.7.0_80/jre/lib/rt.jar

cd /data/s4040112/sourcecodes/mina

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
if [ "$projectpath" = "" ] ; then
echo "Project path is NOT set"
return
fi
#source /etc/profile


#don't forget to run sudo updatedb, since locate finds all files but needs to be updated using this command

find ${projectpath} -name '*.java' > ${projectname}-files.list
#locate ${projectpath}**.java > ${projectname}-files.list

mvn dependency:copy-dependencies -DoutputDirectory=dependencies -Dhttps.protocols=TLSv1.2

find ${projectpath} -name '*.jar' > ${projectname}-jars.list

last_line=$(wc -l < ${projectname}-jars.list)
current_line=0

while read line
  do
    export CLASSPATH=${CLASSPATH}:$line
done < ${projectname}-jars.list

java -jar /data/s4040112/Internship_RuG_2020/0-ProjectRefactorer/out/artifacts/0_ProjectRefactorer_jar/0-ProjectRefactorer.jar $projectname

if [ "$verbose" = true ] ; then
echo "$(<${projectname}-files.list)"
fi
pinot @${projectname}-newfiles.list 2>&1 | tee "pinot-ergebnis-${projectname}.txt"

rm ${projectname}-files.list
rm ${projectname}-newfiles.list
rm ${projectname}-jars.list

find ${projectpath} -name '*refactored.java' > ${projectname}-deletefiles.list

while read line
  do
	rm $line
  done < ${projectname}-deletefiles.list
