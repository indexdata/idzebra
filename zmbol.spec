Name: zmbol
Version: 1.1
Release: 1
Requires: yaz
Copyright: Commercial
Group: Applications/Databases
Vendor: Index Data ApS <info@indexdata.dk>
Source: zmbol-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-root
Packager: Adam Dickmeiss <adam@indexdata.dk>
URL: http://www.indexdata.dk/zmbol/
Summary: Z'mbol: a fielded free-text engine with a Z39.50 frontend.

%description
Zmbol is a fielded free-text indexing and retrieval engine with a Z39.50
frontend. You can use any compatible, commercial or freeware Z39.50 client to
access data stored in Zmbol.

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
%doc README LICENSE.zmbol CHANGELOG 
%config /usr/share/zmbol/tab
/usr/bin/zmbolsrv
/usr/bin/zmbolidx
/usr/share/zmbol/doc
