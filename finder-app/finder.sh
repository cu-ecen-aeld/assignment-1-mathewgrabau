#!/usr/bin/sh

# Script requirements: first argument is a path to a directory on the file system (filesdir).
# The second script is for searching the files in the directory (searchstr)

if [ $# -eq 2 ]
then
    if [ -d "$1" ] 
    then 
        # Need to print the number of files in directory and the number of matching lines in the directory
        FILECOUNT=$(find $1 -type f | wc -l)
        MATCHCOUNT=$(grep -R $2 $1 | wc -l)
        echo "The number of files are ${FILECOUNT} and the number of matching lines are ${MATCHCOUNT}"
    else
        echo "$1 is not a valid directory"
        exit 1
    fi
else
    echo "Usage: finder.sh filesdir searchstr --> requires two arguments."
    exit 1
fi


