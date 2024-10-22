#disable LTO otherwise compilation fails on centos9
%global _lto_cflags %nil

%define idmetaversion %(. ./IDMETA; echo $VERSION)
Name: idzebra
%define namev idzebra-2.0
Version: %{idmetaversion}
Release: 1.indexdata
License: GPL
Vendor: Index Data ApS <info@indexdata.dk>
Source: idzebra-%{version}.tar.gz
BuildRoot: %{_tmppath}/idzebra-%{version}-root
Packager: Adam Dickmeiss <adam@indexdata.dk>
URL: http://www.indexdata.com/zebra
BuildRequires: libyaz5-devel >= 5.29.0
BuildRequires: expat-devel, bzip2-devel, tcl, zlib-devel, pkgconfig
Summary: High-performance, structured text indexing and retrival engine.
Group: Applications/Databases
%description
Zebra is a high-performance, general-purpose structured text indexing
and retrieval engine. It reads structured records in a variety of input
formats (eg. email, XML, MARC) and allows access to them through exact
boolean search expressions and relevance-ranked free-text queries. 

%package -n %{namev}
Summary: High-performance, structured text indexing and retrival engine.
Group: Applications/Databases
Requires: lib%{namev}-modules = %{version}
%description -n %{namev}
Zebra is a high-performance, general-purpose structured text indexing
and retrieval engine. It reads structured records in a variety of input
formats (eg. email, XML, MARC) and allows access to them through exact
boolean search expressions and relevance-ranked free-text queries. 


%package -n lib%{namev}
Summary: Zebra libraries
Group: Libraries
Requires: libyaz5 bzip2-libs
%description -n lib%{namev}
Libraries for the Zebra search engine.

%package -n lib%{namev}-modules
Summary: Zebra modules
Group: Libraries
Requires: lib%{namev} = %{version} expat tcl
%description -n lib%{namev}-modules
Modules for the Zebra search engine.

%package -n lib%{namev}-devel
Summary: Zebra development libraries
Group: Development/Libraries
Requires: lib%{namev} = %{version} libyaz5-devel bzip2-devel 
%description -n lib%{namev}-devel
Development libraries for the Zebra search engine.

%prep
%setup

%build

CFLAGS="$RPM_OPT_FLAGS" \
 ./configure --prefix=/usr --libdir=%{_libdir} --mandir=%{_mandir}\
	--enable-shared --with-yaz=pkg
%if %{?make_build:1}%{!?make_build:0}
%make_build
%else
make -j4 CFLAGS="$RPM_OPT_FLAGS"
%endif

%install
rm -fr ${RPM_BUILD_ROOT}
make install DESTDIR=${RPM_BUILD_ROOT}
rm ${RPM_BUILD_ROOT}/%{_libdir}/*.la
rm ${RPM_BUILD_ROOT}/%{_bindir}/zebraidx
rm ${RPM_BUILD_ROOT}/%{_mandir}/man1/zebraidx.*
rm ${RPM_BUILD_ROOT}/%{_bindir}/zebrasrv
rm ${RPM_BUILD_ROOT}/%{_mandir}/man8/zebrasrv.*
rm ${RPM_BUILD_ROOT}/%{_mandir}/man1/idzebra-config.*
mkdir -p ${RPM_BUILD_ROOT}/etc/idzebra
mkdir -p ${RPM_BUILD_ROOT}/etc/rc.d/init.d
install -m755 rpm/zebrasrv.init ${RPM_BUILD_ROOT}/etc/rc.d/init.d/zebrasrv
mkdir -p ${RPM_BUILD_ROOT}/etc/logrotate.d
install -m644 rpm/zebrasrv.logrotate ${RPM_BUILD_ROOT}/etc/logrotate.d/zebrasrv

%clean
rm -fr ${RPM_BUILD_ROOT}

%files -n %{namev}
%defattr(-,root,root)
%doc README.md LICENSE.zebra NEWS
%config /usr/share/idzebra-2.0/tab
%{_bindir}/zebrasrv-*
%{_bindir}/zebraidx-*
%{_bindir}/idzebra-abs2dom*
/usr/share/doc/idzebra-2.0
%{_mandir}/*/zebraidx-*
%{_mandir}/*/zebrasrv-*
%{_mandir}/*/idzebra-abs2dom*
/usr/share/idzebra-2.0-examples
%dir %{_sysconfdir}/idzebra
%config %{_sysconfdir}/rc.d/init.d/zebrasrv
%config(noreplace) /etc/logrotate.d/zebrasrv

%files -n lib%{namev}
%{_libdir}/*.so.*

%files -n lib%{namev}-modules
%{_libdir}/idzebra-2.0/modules/*

%files -n lib%{namev}-devel
%{_bindir}/idzebra-config-*
%{_includedir}/idzebra-2.0
%{_libdir}/*.so
%{_libdir}/*.a
%{_mandir}/*/idzebra-config-*
%{_datadir}/aclocal/*.m4
%{_libdir}/pkgconfig/*.pc

%post -n lib%{namev}
/sbin/ldconfig 
%postun -n lib%{namev}
/sbin/ldconfig 
%post -n %{namev}
if [ $1 = 1 ]; then
	/sbin/chkconfig --add zebrasrv
	/sbin/service zebrasrv start > /dev/null 2>&1
else
	/sbin/service zebrasrv restart > /dev/null 2>&1
fi
%preun -n %{namev}
if [ $1 = 0 ]; then
	/sbin/service zebrasrv stop > /dev/null 2>&1
	/sbin/chkconfig --del zebrasrv
fi

