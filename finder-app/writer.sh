#!/bin/sh

writefile=$1
writestr=$2

if [ $# -lt 2 ]
then
	echo "The full path to a file or the text string is not specified"
	exit 1
fi


if [ ! -d "$writefile" ] 
then
	mkdir -p "$(dirname "$writefile")" && touch "$writefile"
fi

echo "$writestr" > "$writefile"

if [ $? -ne 0 ]
then
   echo "Error: Failed to create file"
   exit 1
fi
