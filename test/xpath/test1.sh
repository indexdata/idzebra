#!/bin/sh
# $Id: test1.sh,v 1.4 2004-06-15 08:06:35 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log
rm -f $LOG
../../index/zebraidx -c $pp/zebra.cfg -l $LOG init || exit 1
../../index/zebraidx -c $pp/zebra.cfg -l $LOG -t grs.sgml update rec.xml || exit 2
