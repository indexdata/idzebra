#!/bin/sh
# $Id: buildconf.sh,v 1.7 2002-04-05 08:46:26 adam Exp $
dir=`aclocal --print-ac-dir`
if [ -f $dir/yaz.m4 ]; then
	aclocal
else
	aclocal -I .
fi
automake -a >/dev/null 2>&1 
autoconf
if [ -f config.cache ]; then
	rm config.cache
fi
