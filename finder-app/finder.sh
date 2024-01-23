#!/bin/bash
#Author:Isha Sharma

#check input params, lt-less than
if [ $# -lt 2 ]; then
    echo "Error : 2 required aprameters, not specified"
    exit 1
fi

# parameters into varibales ($1 is first param ....)
filesdir=$1
searchstr=$2

number_of_files=0
number_of_lines=0

#to check if filesdir exists
if [ ! -d "$filesdir" ]; then
	echo "Filesdir does not exist as a directory"
	exit 1
fi

number_of_files=$(find "$filesdir" -type f | wc -l)
number_of_lines=$(grep -r "$searchstr" "$filesdir" | wc -l)

echo "The number of files are $number_of_files and the number of matching lines are $number_of_lines"
