#!/bin/sh
t=$1
test -n "$t" || exit 1
rm -f *.mf *.LCK *.tmp
../../index/zebraidx -l zebraidx.log init 
i=0
rm -f times-$t.log
while test -f dmoz.$i.xml; do
	echo -n "$i " >>times-$1.log
	/usr/bin/time -f '%e %U %P' -a -o times-$t.log ../../index/zebraidx -l zebraidx.log -c zebra-$t.cfg -f 10 update dmoz.$i.xml
	../../index/zebraidx -l zebraidx.log -c zebra-$t.cfg stat
	i=`expr $i + 1`
	if test $i = 29; then
		break
	fi
done
