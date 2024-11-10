#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: $0 test_pattern output_filename"
    exit -1
fi

touch $2
minimumsize=65536
until [ $(wc -c < $2) -ge $minimumsize ];
do
    fortune >> $2
done
echo $2 is $(wc -c < $2) bytes

cat $2 | grep $1 | more > $2_against_$1
