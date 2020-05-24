projectpath="/data/s4040112/mina"
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
if [[ $CLASSPATH != *"rt.jar"* ]] ; then
export CLASSPATH=${CLASSPATH}
#:/usr/lib/jvm/jdk1.6.0_45/jre/lib/\
#:/usr/lib/jvm/jdk1.6.0_45/jre/lib/rt.jar\
#:/usr/lib/jvm/jdk1.6.0_45/jre/bin/\
:/data/s4040112/Internship_RuG_2020/Scripts_and_Tools/pinot-src/lib/rt.jar
#fi

#don't forget to run sudo updatedb, since locate finds all files but needs to be updated using this command
updatedb

find ${projectpath} -name '*.java' > ${projectname}-files.list
#locate ${projectpath}**.java > ${projectname}-files.list

if [ "$verbose" = true ] ; then
echo "$(<${projectname}-files.list)"
fi
pinot @${projectname}-files.list 2>&1 | tee "pinot-ergebnis-${projectname}.txt"
#rm ${projectname}-files.list