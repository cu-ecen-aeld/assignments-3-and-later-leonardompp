#!/bin/sh

# Check that exactly two arguments are provided
if [ $# -ne 2 ]; then
    echo "Error: Two arguments required: <FILESDIR> <SEARCHSTR>"
    exit 1
fi

# Assign arguments to variables
FILESDIR=$1
SEARCHSTR=$2

# Check that FILESDIR is a valid directory
if [ ! -d ${FILESDIR} ]; then
    echo "Error: ${FILESDIR} is not a directory or does not exist"
    exit 1
fi

# Count the number of files under filesdir
NUMFILES=$(find ${FILESDIR} -type f | wc -l)

# Count the number of matching lines containing searchstr
NUM_MATCHING_LINES=$(grep -R ${SEARCHSTR} ${FILESDIR} 2>/dev/null | wc -l)

# Print the required message
echo "The number of files are ${NUMFILES} and the number of matching lines are ${NUM_MATCHING_LINES}"
