#!/bin/sh
# $Id: test1.sh,v 1.3 2003-05-21 14:39:23 adam Exp $
LOG=test1.log
rm -f $LOG
../../index/zebraidx -l $LOG init || exit 1
../../index/zebraidx -l $LOG -t grs.sgml update rec.xml || exit 2
