#!/bin/sh
# $Id: buildconf.sh,v 1.4 2001-02-26 22:14:59 adam Exp $
dir=`aclocal --print-ac-dir`
if [ -f $dir/yaz.m4 ]; then
	aclocal || exit 1
else
	aclocal -I . || exit 1
fi
automake -a >/dev/null 2>&1 || exit 2
autoconf || exit 3
if [ -f config.cache ]; then
	rm config.cache
fi
