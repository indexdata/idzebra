#!/bin/sh
# $Id: test1.sh,v 1.3 2004-06-15 08:06:34 adam Exp $

pp=${srcdir:-"."}

LOG=test1.log
../../index/zebraidx -c $pp/zebra.cfg -l $LOG init
../../index/zebraidx -c $pp/zebra.cfg -l $LOG update g.rec
