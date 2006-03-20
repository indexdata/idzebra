#!/bin/sh
# $Id: buildconf.sh,v 1.15 2006-03-20 09:30:13 mike Exp $
set -x
dir=`aclocal --print-ac-dir`
aclocal -I .
libtoolize --automake --force 
automake -a 
automake -a 
autoconf
set -
if [ -f config.cache ]; then
	rm config.cache
fi

enable_configure=false
enable_help=true
sh_flags=""
conf_flags=""
case $1 in
    -d)
	sh_flags="-g -Wall -O0"
	enable_configure=true
	enable_help=false
	shift
	;;
    -c)
	sh_flags=""
	enable_configure=true
	enable_help=false
	shift
	;;
esac

if $enable_configure; then
    if test -n "$sh_flags"; then
	CFLAGS="$sh_flags" ./configure $*
    else
	./configure $*
    fi
fi
if $enable_help; then
    cat <<EOF

Build the Makefiles with the configure command.
  ./configure [--someoption=somevalue ...]

For help on options or configuring run
  ./configure --help

Build and install binaries with the usual
  make
  make check
  make install

Build distribution tarball with
  make dist

Verify distribution tarball with
  make distcheck

Or just build the Debian packages without configuring
  dpkg-buildpackage -rfakeroot

When building from a CVS checkout, you need these Debian tools:
  docbook-utils, docbook, docbook-xml, docbook-dsssl, jade, jadetex,
  libyaz-dev, libexpat1-dev, libtcl8.3-dev, libbz2-dev
and if you want the Alvis/XSLT filter, you also need:
  libxslt1-dev

To build against a YAZ in an adjacent dirctory, use:
	CFLAGS="-g -O0" LDFLAGS="-L`pwd`/../yaz/src/.libs" CPPFLAGS="-I../yaz/include" ./configure
EOF
fi
