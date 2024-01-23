#!/bin/bash
#Author: Isha Sharma

#check input params, lt-less than 2
if [ $# -lt 2 ]; then
    echo "Error : 2 required parameters, not specified"
    exit 1
fi

# parameters into varibales ($1 is first param ....)
writefile=$1
writestr=$2


# Extract the directory path from writefile
dirpath=$(dirname "$writefile")

# Create the directory path if it doesn't exist
if [ ! -d "$dirpath" ]; then
    mkdir -p "$dirpath" || { echo "Error: Could not create directory path '$dirpath'."; exit 1; }
fi

echo "$writestr" > "$writefile"

if [ "$?" -ne 0 ]; then
    echo "Error: file could not be created."
    exit 1
fi
