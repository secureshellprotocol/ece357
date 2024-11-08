#!/bin/bash

if [ $# -ne 1 ];
    then
    echo "Usage: $0 output_filename"
    exit -1
fi

touch $1
minimumsize=65536
actualsize=$(wc -c < $1)
until [ $(wc -c < $1) -ge $minimumsize ];
do
    fortune >> $1
done
