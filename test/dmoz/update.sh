#!/bin/sh
# $Id: update.sh,v 1.8 2002-06-20 08:03:30 adam Exp $
t=$1
test -n "$t" || exit 1
rm -f *.mf *.LCK *.tmp
../../index/zebraidx -l zebraidx-$t.log init 
i=0
rm -f times-$t.log stat-$t.log
while test -f dmoz.$i.xml; do
	echo -n "$i " >>times-$1.log
	rm -f zebraidx-$t.log
	../../index/zebraidx -l zebraidx-$t.log -c zebra-$t.cfg -f 10 update dmoz.$i.xml 

	grep ' zebraidx times:' zebraidx-$t.log | sed 's/.*zebraidx times://g' >>times-$t.log

	test -d rtmp || mkdir rtmp
	cp *.mf rtmp
	rm -f *.mf
	mv rtmp/*.mf .

	echo " ---------- $i ----- " >> stat-$t.log
	../../index/zebraidx -l zebraidx-$t.log -c zebra-$t.cfg stat >>stat-$t.log 
	i=`expr $i + 1`
	if test $i = 30; then
		break
	fi
done

title="ISAM-$t `date`"

cat >plot.dem <<ENDOFMESSAGE
set output "times-$t.ps"
set terminal postscript
set title "$title"
set xlabel "runs"
set ylabel "seconds"
plot [0:] [0:] \
        'times-$t.log' using 2 title 'real' with linespoints, \
        'times-$t.log' using 3 title 'user' with linespoints, \
        'times-$t.log' using 4 title 'sys' with linespoints
ENDOFMESSAGE

gnuplot plot.dem


