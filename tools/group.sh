#!/bin/bash

while read line
do
	IFS=',' read -a arr <<< "${line}"
	#arr=$(echo $line | tr "," "\n")
	#echo ${arr[0]}
	grep ${arr[0]} groups.csv &> /dev/null
	if [ "1" -eq "$?" ]
	then
		echo ${arr[@]}
	fi
done < Registration_list.csv

