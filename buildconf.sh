#!/bin/sh
# $Id: buildconf.sh,v 1.5 2001-02-28 09:01:41 adam Exp $
dir=`aclocal --print-ac-dir`
if [ -f $dir/yaz.m4 ]; then
	aclocal || exit 1
else
	aclocal -I . || exit 1
fi
test -d isamc || mkdir isamc
test -d isamb || mkdir isamb
test -d isam || mkdir isam
automake -a >/dev/null 2>&1 || exit 2 
autoconf || exit 3
if [ -f config.cache ]; then
	rm config.cache
fi
