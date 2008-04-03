#!/bin/sh

pp=${srcdir:-"."}

LOG=test1.log
../../index/zebraidx -c $pp/zebra.cfg -l $LOG init
../../index/zebraidx -c $pp/zebra.cfg -l $LOG update $pp/g.rec
