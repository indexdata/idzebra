#!/bin/sh
# run all zebrash tests
mkdir -p reg
rm -f *.mf reg/*.mf *.out

for F in *.zsh
do
  echo $F
  ../../index/zebrash <$F >$F.out 
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
echo OK
exit 0
