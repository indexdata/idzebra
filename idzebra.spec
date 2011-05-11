Name: idzebra-2.0
Version: 2.0.46
Release: 1indexdata
Requires: lib%{name}-modules = %{version}
License: GPL
Group: Applications/Databases
Vendor: Index Data ApS <info@indexdata.dk>
Source: idzebra-%{version}.tar.gz
BuildRoot: %{_tmppath}/idzebra-%{version}-root
Packager: Adam Dickmeiss <adam@indexdata.dk>
URL: http://www.indexdata.dk/zebra/
BuildRequires: libyaz4-devel expat-devel bzip2-devel tcl zlib-devel
Summary: High-performance, structured text indexing and retrival engine.

%description
Zebra is a high-performance, general-purpose structured text indexing
and retrieval engine. It reads structured records in a variety of input
formats (eg. email, XML, MARC) and allows access to them through exact
boolean search expressions and relevance-ranked free-text queries. 

%package -n lib%{name}
Summary: Zebra libraries
Group: Libraries
Requires: libyaz4 bzip2-libs
%description -n lib%{name}
Libraries for the Zebra search engine.
%post -p /sbin/ldconfig 
%postun -p /sbin/ldconfig 

%package -n lib%{name}-modules
Summary: Zebra modules
Group: Libraries
Requires: lib%{name} = %{version} expat tcl
%description -n lib%{name}-modules
Modules for the Zebra search engine.

%package -n lib%{name}-devel
Summary: Zebra development libraries
Group: Development/Libraries
Requires: lib%{name} = %{version} libyaz4-devel bzip2-devel 
%description -n lib%{name}-devel
Development libraries for the Zebra search engine.

%prep
%setup -n idzebra-%{version}

%build

CFLAGS="$RPM_OPT_FLAGS" \
 ./configure --prefix=/usr --enable-shared --with-yaz=/usr/bin
make CFLAGS="$RPM_OPT_FLAGS"

%install
rm -fr ${RPM_BUILD_ROOT}
make prefix=${RPM_BUILD_ROOT}/usr mandir=${RPM_BUILD_ROOT}/usr/share/man install
rm ${RPM_BUILD_ROOT}/usr/lib/*.la
rm ${RPM_BUILD_ROOT}/usr/bin/zebraidx
rm ${RPM_BUILD_ROOT}/usr/share/man/man1/zebraidx.*
rm ${RPM_BUILD_ROOT}/usr/bin/zebrasrv
rm ${RPM_BUILD_ROOT}/usr/share/man/man8/zebrasrv.*
rm ${RPM_BUILD_ROOT}/usr/share/man/man1/idzebra-config.*

%clean
rm -fr ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%doc README LICENSE.zebra NEWS
%config /usr/share/idzebra-2.0/tab
/usr/bin/zebrasrv-*
/usr/bin/zebraidx-*
/usr/share/doc/idzebra-2.0
/usr/share/man/*/zebraidx-*
/usr/share/man/*/zebrasrv-*
/usr/share/idzebra-2.0-examples
%files -n lib%{name}
/usr/lib/*.so.*

%files -n lib%{name}-modules
/usr/lib/idzebra-2.0/modules/*

%files -n lib%{name}-devel
/usr/bin/idzebra-config-*
/usr/include/idzebra-2.0/*
/usr/lib/*.so
/usr/lib/*.a
/usr/share/man/*/idzebra-config-*
/usr/share/aclocal/*.m4
