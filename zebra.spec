Summary: Zebra - An Open Indexing Search Engine
Name: zebra
Version: 1.0
Release: 1
Copyright: commercial
Group: Development/Libraries
Vendor: Index Data ApS <info@indexdata.dk>
Url: http://www.indexdata.dk/zebra/
Source: zebra-1.0.tar.gz
Requires: yaz
BuildRequires: yaz
BuildRoot: /var/tmp/%{name}-%{version}-root
Packager: Adam Dickmeiss <adam@indexdata.dk>

%description
The Zebra indexing tool is a generic search engine suitable for dealing
with Metadata and other document types. Zebra consists of an indexing
tool - for building an index for fast retrieval - and a search engine
server that offers a Z39.50 interface.

%prep
%setup

%build

CFLAGS="$RPM_OPT_FLAGS" \
 ./configure --with-build-root=$RPM_BUILD_ROOT --prefix=/usr 
make CFLAGS="$RPM_OPT_FLAGS"

%install
rm -fr $RPM_BUILD_ROOT
make install

%files
%defattr(-,root,root)
%doc README LICENSE.2 CHANGELOG
%config /usr/lib/zebra/tab
/usr/bin/zebraidx
/usr/bin/zebrasrv
%dir /usr/lib/zebra
