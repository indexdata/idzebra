#!/bin/sh
# $Id: testall.sh,v 1.3 2004-06-15 08:06:36 adam Exp $
# run all zebrash tests

pp=${srcdir:-"."}

test -d reg || mkdir reg
rm -f *.mf reg/*.mf *.out

for F in *.zsh
do
  echo $F
  ../../index/zebrash -c $pp/zebra.cfg <$F >$F.out 
  RC=$?
  if [ "$RC" -gt "0" ]
  then 
    echo "$F failed with exit code $RC"
    FINAL=$RC
  fi
done
if [ "$FINAL" ]
then
    echo "Tests FAILED"
    exit 9
fi
exit 0
