#!/bin/sh
# $Id: buildconf.sh,v 1.6 2002-04-04 14:14:13 adam Exp $
dir=`aclocal --print-ac-dir`
if [ -f $dir/yaz.m4 ]; then
	aclocal
else
	aclocal -I .
fi
test -d isamc || mkdir isamc
test -d isamb || mkdir isamb
test -d isam || mkdir isam
automake -a >/dev/null 2>&1 
autoconf
if [ -f config.cache ]; then
	rm config.cache
fi
