#!/bin/sh
# $Id: test1.sh,v 1.4 2004-06-15 09:43:30 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log
../../index/zebraidx -c $pp/zebra.cfg -l $LOG init
../../index/zebraidx -c $pp/zebra.cfg -l $LOG update $pp/g.rec
