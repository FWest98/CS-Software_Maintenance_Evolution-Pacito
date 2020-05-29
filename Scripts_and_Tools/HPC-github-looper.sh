#!/bin/bash
#SBATCH --time=10-00:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --job-name=pinot_mina
#SBATCH --mem=100GB
#SBATCH --mail-type=ALL
#SBATCH --mail-user=f.a.de.capela@student.rug.nl
#SBATCH --output=job-%j-pinot_mina.log
#SBATCH --partition=regular

module load Java/1.7.0_80
export CLASSPATH=${CLASSPATH}:/apps/generic/software/Java/1.7.0_80/jre/lib/rt.jar

COUNTER=1

cd /home/s4040112/sourcecodes/mina/
git pull
FINAL_COMMIT=$(git show -s --format=%H)

#depending on the project, the main branch can either be master or trunk
#git rev-list --reverse master > commitOrder.txt
git rev-list --reverse trunk > commitOrder.txt

filename=commitOrder.txt
file_lines=`cat $filename`

#mkdir -p outputs

projectpath="/home/s4040112/sourcecodes/mina/"
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
#	sh /home/p289550/tools/Pinot/HPC-pinotscript.sh 2>&1 | tee /data/p289550/Pinot_results/Mina_results/$COUNTER-ID-$CURRENT_COMMIT.txt

#don't forget to run sudo updatedb, since locate finds all files but needs to be updated using this command
updatedb

find ${projectpath} -name '*.java' > ${projectname}-files.list
#locate ${projectpath}**.java > ${projectname}-files.list

if [ "$verbose" = true ] ; then
echo "$(<${projectname}-files.list)"
fi
/home/s4040112/tools/bin/pinot @${projectname}-files.list 2>&1 | tee /data/s4040112/Pinot_results/Mina_results/$COUNTER-ID-$CURRENT_COMMIT.txt


	COUNTER=$((COUNTER+1))
	git log -1 --pretty=format:"%h - %an, %ar"
	echo $(git log $CURRENT_COMMIT..$FINAL_COMMIT --pretty=oneline | wc -l) " - Number of commits left"
done
