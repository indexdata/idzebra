Name: idzebra-2.0
Version: 2.0.49
Release: 1indexdata
Requires: lib%{name}-modules = %{version}
License: GPL
Group: Applications/Databases
Vendor: Index Data ApS <info@indexdata.dk>
Source: idzebra-%{version}.tar.gz
BuildRoot: %{_tmppath}/idzebra-%{version}-root
Packager: Adam Dickmeiss <adam@indexdata.dk>
URL: http://www.indexdata.dk/zebra/
BuildRequires: libyaz4-devel >= 4.2.0
BuildRequires: expat-devel, bzip2-devel, tcl, zlib-devel
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
 ./configure --prefix=/usr --libdir=%{_libdir} --mandir=%{_mandir}\
	--enable-shared --with-yaz=/usr/bin
make CFLAGS="$RPM_OPT_FLAGS"

%install
rm -fr ${RPM_BUILD_ROOT}
make prefix=${RPM_BUILD_ROOT}/usr mandir=${RPM_BUILD_ROOT}/%{_mandir} \
        libdir=${RPM_BUILD_ROOT}/%{_libdir} install
rm ${RPM_BUILD_ROOT}/%{_libdir}/*.la
rm ${RPM_BUILD_ROOT}/%{_bindir}/zebraidx
rm ${RPM_BUILD_ROOT}/%{_mandir}/man1/zebraidx.*
rm ${RPM_BUILD_ROOT}/%{_bindir}/zebrasrv
rm ${RPM_BUILD_ROOT}/%{_mandir}/man8/zebrasrv.*
rm ${RPM_BUILD_ROOT}/%{_mandir}/man1/idzebra-config.*

%clean
rm -fr ${RPM_BUILD_ROOT}

%files
%defattr(-,root,root)
%doc README LICENSE.zebra NEWS
%config /usr/share/idzebra-2.0/tab
%{_bindir}/zebrasrv-*
%{_bindir}/zebraidx-*
%{_bindir}/idzebra-abs2dom*
/usr/share/doc/idzebra-2.0
%{_mandir}/*/zebraidx-*
%{_mandir}/*/zebrasrv-*
%{_mandir}/*/idzebra-abs2dom*
/usr/share/idzebra-2.0-examples

%files -n lib%{name}
%{_libdir}/*.so.*

%files -n lib%{name}-modules
%{_libdir}/idzebra-2.0/modules/*

%files -n lib%{name}-devel
%{_bindir}/idzebra-config-*
%{_includedir}/idzebra-2.0
%{_libdir}/*.so
%{_libdir}/*.a
%{_mandir}/*/idzebra-config-*
/usr/share/aclocal/*.m4

