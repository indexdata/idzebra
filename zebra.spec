Name: zebra
Version: 1.3.1
Release: 1
Requires: yaz expat bzip2-libs tcl
Copyright: GPL
Group: Applications/Databases
Vendor: Index Data ApS <info@indexdata.dk>
Source: zebra-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-root
Packager: Adam Dickmeiss <adam@indexdata.dk>
URL: http://www.indexdata.dk/zebra/
BuildRequires: yaz expat-devel bzip2-devel tcl
Summary: High-performance, structured text indexing and retrival engine.

%description
Zebra is a high-performance, general-purpose structured text indexing
and retrieval engine. It reads structured records in a variety of input
formats (eg. email, XML, MARC) and allows access to them through exact
boolean search expressions and relevance-ranked free-text queries. 

%prep
%setup

%build

CFLAGS="$RPM_OPT_FLAGS" \
 ./configure --prefix=/usr --with-yazconfig=/usr/bin
make CFLAGS="$RPM_OPT_FLAGS"

%install
rm -fr $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT/usr install
cd doc; make prefix=$RPM_BUILD_ROOT/usr install

%files
%defattr(-,root,root)
%doc README LICENSE.zebra CHANGELOG 
%config /usr/share/zebra/tab
/usr/bin/zebrasrv
/usr/bin/zebraidx
/usr/share/doc/zebra
