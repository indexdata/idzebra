#!/bin/sh

if [ -d .git ]; then
    git submodule init
    git submodule update
fi

. m4/id-config.sh

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

EOF
    if [ -f /etc/debian_version ]; then
        cat <<EOF
Or just build the Debian packages without configuring
  dpkg-buildpackage -rfakeroot

When building from Git, you need these Debian packages:
  autoconf automake libtool gcc tclsh
  xsltproc docbook docbook-xml docbook-xsl librsvg2-bin
  libxslt1-dev libgnutls-dev libreadline5-dev libexpat1-dev
  pkg-config libexpat1-dev

And if you want to make a Debian package: dpkg-dev fakeroot debhelper
(Then run "dpkg-buildpackage -rfakeroot" in this directory.)

EOF
    fi
    if [ "`uname -s`" = FreeBSD ]; then
        cat <<EOF
When building from a Git, you need these FreeBSD Ports:
  pkg_add -r autoconf268 automake111 libtool tcl84 \\
             docbook-xsl libxml2 libxslt g++-4.0 make
EOF
    fi
fi
