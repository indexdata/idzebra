#!/bin/sh
# run all zebrash tests

srcdir=${srcdir:-"."}


test -d reg || mkdir reg
rm -f *.mf reg/*.mf *.out

for F in $srcdir/*.zsh
do
  echo $F
  if [ "." = "$srcdir" ] 
      then  # running make check
      ../../index/zebrash -c $srcdir/zebra.cfg <$F >$F.out
  else   # running make distcheck
      ../../index/zebrash -c $srcdir/zebra.cfg <$F >/dev/null
  fi
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
