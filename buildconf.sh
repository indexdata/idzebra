#!/bin/sh
# $Id: buildconf.sh,v 1.12 2004-09-27 10:44:47 adam Exp $
set -x
dir=`aclocal --print-ac-dir`
aclocal -I .
libtoolize --automake --force 
automake -a 
automake -a 
autoconf
if [ -f config.cache ]; then
	rm config.cache
fi
