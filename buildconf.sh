#!/bin/sh
# $Id: buildconf.sh,v 1.8 2002-04-30 21:07:56 adam Exp $
dir=`aclocal --print-ac-dir`
if [ -f $dir/yaz.m4 ]; then
	aclocal
else
	aclocal -I .
fi
automake -a 
autoconf
if [ -f config.cache ]; then
	rm config.cache
fi
