#!/bin/bash
projectpath="/home/filipe/Desktop/Internship_RuG_2020/Analyzed_Projects/"
projectname=""
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

#if [[ $CLASSPATH != *"rt.jar"* ]] ; then
export CLASSPATH=
export CLASSPATH=${CLASSPATH}:/usr/lib/jvm/java-7-openjdk-amd64/jre/lib/rt.jar
#fi

#don't forget to run sudo updatedb, since locate finds all files but needs to be updated using this command

find ${projectpath} -name '*.java' > ${projectname}-files.list
#locate ${projectpath}**.java > ${projectname}-files.list

java -jar ~/Desktop/Internship_RuG_2020/0-ProjectRefactorer/out/artifacts/0_ProjectRefactorer_jar/0-ProjectRefactorer.jar $projectname delete

if [ "$verbose" = true ] ; then
echo "$(<${projectname}-files.list)"
fi
pinot @${projectname}-newfiles.list 2>&1 | tee "pinot-ergebnis-${projectname}.txt"
#rm ${projectname}-files.list


