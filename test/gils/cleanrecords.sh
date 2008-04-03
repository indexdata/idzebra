#!/bin/sh

srcdir=${srcdir:-"."}

# these should not be here, will be created later
if [ -f $srcdir/records/esdd0001.grs ] 
    then
    rm -f $srcdir/records/esdd0001.grs
fi
if [ -f $srcdir/records/esdd0002.grs ] 
    then
    rm -f $srcdir/records/esdd0002.grs
fi
