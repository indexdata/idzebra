#!/bin/sh
# $Id: buildconf.sh,v 1.11 2004-08-24 10:37:18 adam Exp $
set -x
dir=`aclocal --print-ac-dir`
if [ -f $dir/yaz.m4 ]; then
	aclocal
else
	aclocal -I .
fi
libtoolize --automake --force 
automake -a 
automake -a 
autoconf
if [ -f config.cache ]; then
	rm config.cache
fi
