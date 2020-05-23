#!/bin/bash
#SBATCH --time=10-00:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --job-name=pinot_mina
#SBATCH --mem=32GB
#SBATCH --mail-type=ALL
#SBATCH --mail-user=f.a.de.capela@students.rug.nl
#SBATCH --output=job-%j-pinot_mina.log
#SBATCH --partition=regular

#Modules Needed
# GNU Make
# GCC
# G++
# Java

echo Setting up repository

cd /data/s4040112
git clone https://github.com/FilipeCapela98/Internship_RuG_2020.git
cd Internship_RuG_2020/Scripts_and_Tools/pinot_src
chmod -R +rwx /data/s4040112/Internship_RuG_2020


echo Pinot Installation

./configure --prefix=/home/s4040112/tools
make 
make install


echo Clone project to be analyzed repository

cd /data/s4040112/
git clone https://github.com/apache/mina.git
chmod -R +rwx /data/s4040112/mina

cd /data/s4040112/mina



#################
# GITHUB-LOOPER #
#################

COUNTER=1

git pull
FINAL_COMMIT=$(git show -s --format=%H)

#depending on the project, the main branch can either be master or trunk
#git rev-list --reverse master > commitOrder.txt
git rev-list --reverse trunk > commitOrder.txt

filename=commitOrder.txt
file_lines=`cat $filename`

mkdir -p outputs

for line in $file_lines ; 
do
	git reset --hard $line
    	CURRENT_COMMIT=$(git log -n1 --format=format:"%H")
	


		#./pinotscript.sh 2>&1 | tee outputs/$COUNTER-ID-$CURRENT_COMMIT.txt



	COUNTER=$((COUNTER+1))
	git log -1 --pretty=format:"%h - %an, %ar"
	echo $(git log $CURRENT_COMMIT..$FINAL_COMMIT --pretty=oneline | wc -l) " - Number of commits left"
done


###############
# PINOTSCRIPT #
###############



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


