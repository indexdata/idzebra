#!/bin/sh
# $Id: test1.sh,v 1.5 2004-06-15 09:43:34 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log
rm -f $LOG
../../index/zebraidx -c $pp/zebra.cfg -l $LOG init || exit 1
../../index/zebraidx -c $pp/zebra.cfg -l $LOG -t grs.sgml update $pp/rec.xml || exit 2
