#!/bin/sh
# $Id: buildconf.sh,v 1.2 2001-02-21 11:22:54 adam Exp $
dir=`aclocal --print-ac-dir`
if [ -f $dir/yaz.m4 ]; then
	aclocal
else
	aclocal -I .
fi
automake -a
autoconf
