#!/bin/sh
# $Id: buildconf.sh,v 1.3 2001-02-26 21:21:50 adam Exp $
dir=`aclocal --print-ac-dir`
if [ -f $dir/yaz.m4 ]; then
	aclocal || exit 1
else
	aclocal -I . || exit 1
fi
automake -a >/dev/null 2>&1 || exit 2
autoconf || exit 3
