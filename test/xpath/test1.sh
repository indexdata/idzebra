#!/bin/sh
# $Id: test1.sh,v 1.2 2003-05-06 17:39:01 adam Exp $
LOG=test1.log
rm -f $LOG
../../index/zebraidx -l $LOG init || exit 1
../../index/zebraidx -l $LOG -t grs.sgml update rec || exit 2
