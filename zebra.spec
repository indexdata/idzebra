Name: zebra
Version: 1.1.3
Release: 1
Requires: yaz
Copyright: Distributable
Group: Applications/Databases
Vendor: Index Data ApS <info@indexdata.dk>
Source: zebra-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-root
Packager: Adam Dickmeiss <adam@indexdata.dk>
URL: http://www.indexdata.dk/zebra/
Summary: Zebra: a fielded free-text engine with a Z39.50 frontend.

%description
Zebra is a fielded free-text indexing and retrieval engine with a Z39.50
frontend. You can use any compatible, commercial or freeware Z39.50 client to
access data stored in Zebra. Zebra may be used free-of-charge in non-profit
applications by non-commercial organisations. 

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
/usr/share/zebra/doc
