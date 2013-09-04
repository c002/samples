#!/bin/sh -xv

DEST=data
DAYS="01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30"
for d in $DAYS; do
    DATE="$d-Apr-2009"
    echo ./bbb.py -l 2 -s -f $DATE -p $DEST
    ./bbb.py -l 2 -s -f $DATE -p $DEST
done
