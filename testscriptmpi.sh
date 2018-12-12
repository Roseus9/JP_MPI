#!/bin/bash


#if [ "$#" -ne "2" ] ; then
#  echo "Usage: ./[program] [no. of cores] [program]" >&2
#  echo "Example: ./workingscript.sh 5 bakapara" >&2
#  exit 1
#fi

for core in {1..32} ; do
#	sed -i 's/^#define IMAX .*$/#define IMAX 72072 \/\ '$core'/' $1.c
#	if [ "$?" -ne "0" ] ; then
#		echo "sed failed"
#		exit 1
#	fi
	echo "core number : "$core
	mpicc $1.c -o $1.out
	if [ "$?" -eq "0" ] ; then
		if [ "$core" -ge "17" ] ; then
			rsync -avz ~/euler/$1.out 10.112.111.1:~/euler/
			if [ "$?" -ne "0" ] ; then
				echo "rsync:10.112.111.1 failed o run correctly"
				exit 1
			fi
			rsync -avz ~/euler/$1.out 10.112.111.18:~/euler/
			if [ "$?" -ne "0" ] ; then
				echo "rsync:10.112.111.18 failed o run correctly"
				exit 1
			fi
		fi
	else
		echo "mpicc failed to run correctly"
		exit 1
	fi

	for run in {1..3}
	do
	        echo "Run time number: "$run
		mpirun -machinefile ~/machinefile -np $core ./$1.out
		if [ "$?" -ne 0 ] ; then
			echo "mpirun failed"
			exit 1
		fi
	done
done
killall -9 mpirun #this is to ensure the program is killed
