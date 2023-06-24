#!/usr/bin/sh
if [ $# -eq 2 ]
then
    DIRECTORY=$(dirname $1)
    if [ ! -d $DIRECTORY ]
    then
        echo "Creating path ${DIRECTORY}"
        mkdir -p ${DIRECTORY}
    fi
    echo $2 >> $1
else
    echo "Expected 2 arguments, received $#"
    exit 1
fi

