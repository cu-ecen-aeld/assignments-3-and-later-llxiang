#!/bin/sh

filesdir=$1
searchstr=$2

if [ $# -lt 2 ]
then
	echo "The path to the directory or the search string is not specified"
	exit 1
fi

if [ ! -d "$filesdir" ] 
then
	echo "The given path is not a directory on the filesystem"
	exit 1
fi

echo "The number of files are $(find "$filesdir" -type f | wc -l) and the number of matching lines are $(grep -r "$searchstr" "$filesdir" | wc -l)"
