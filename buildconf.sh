#!/bin/sh
# $Id: buildconf.sh,v 1.1 2001-02-21 09:52:39 adam Exp $
aclocal -I .
automake -a
autoconf
