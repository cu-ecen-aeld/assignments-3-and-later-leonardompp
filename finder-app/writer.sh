#!/bin/sh

# Check if both arguments were provided
if [ $# -ne 2 ]; then
    echo "Error: Two arguments required: <WRITEFILE> <WRITESTR>"
    exit 1
fi

WRITEFILE=$1
WRITESTR=$2

# Extract directory path from WRITEFILE
WRITEDIR=$(dirname ${WRITEFILE})

# Create the directory path if it doesn't exist
mkdir -p ${WRITEDIR}
if [ $? -ne 0 ]; then
    echo "Error: Could not create directory path ${WRITEDIR}"
    exit 1
fi

# Create or overwrite the file with WRITESTR
echo ${WRITESTR} > ${WRITEFILE}
if [ $? -ne 0 ]; then
    echo "Error: Could not write to file ${WRITEFILE}"
    exit 1
fi
