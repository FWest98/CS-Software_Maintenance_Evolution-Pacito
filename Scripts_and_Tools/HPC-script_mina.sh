#!/bin/bash
#SBATCH --time=10-00:00:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --job-name=HPC-script_mina
#SBATCH --mem=32GB
#SBATCH --mail-type=ALL
#SBATCH --mail-user=f.a.de.capela@students.rug.nl
#SBATCH --output=job-%j-HPC-script_mina.log
#SBATCH --partition=regular

#Modules Needed
# GNU Make
# GCC
# G++
# Java

#ml load JAVA
#ml load GCC

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

cp /data/s4040112/Internship_RuG_2020/Scripts_and_Tools/HPC-github-looper.sh .
cp /data/s4040112/Internship_RuG_2020/Scripts_and_Tools/HPC-pinotscript .

./HPC-github-looper.sh