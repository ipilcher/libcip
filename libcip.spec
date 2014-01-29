%define	api_ver		0.1
%define so_ver		%{api_ver}.1
%define lib_ver		%{so_ver}.0

Name:		libcip
Summary:	C INI Parser
Version:	%{lib_ver}
Release:	1
License:	GPLv2
Source0:	%{name}-%{version}.tar.gz

%description
libcip is an INI parsing library for C programs.

%package devel
Summary:	Development files for libcip

%description devel
This package contains the files required to build programs that use the
libcip INI parsing library.

%prep
%setup0

%build
gcc -Os -Wall -Wextra -shared -fPIC -fvisibility=hidden \
	-Wl,-soname,%{name}.so.%{so_ver} -o %{name}.so.%{version} *.c types/*.c

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_includedir}
cp %{name}.h %{buildroot}%{_includedir}
mkdir -p %{buildroot}%{_libdir}
cp %{name}.so.%{version} %{buildroot}%{_libdir}
ln -s %{name}.so.%{version} %{buildroot}%{_libdir}/%{name}.so

%post -p /usr/sbin/ldconfig

%postun -p /usr/sbin/ldconfig

%files
%attr(0755,root,root) %{_libdir}/%{name}.so.%{version}

%files devel
%attr(0644,root,root) %{_includedir}/%{name}.h
%attr(0755,root,root) %{_libdir}/%{name}.so

%changelog
* Wed Jan 29 2014 Ian Pilcher <arequipeno@gmail.com> - %{version}
- Version %{version}

* Mon Jan 27 2014 Ian Pilcher <arequipeno@gmail.com> - %{version}
- Hide unnecessary symbols
- Optimize for size
- Rationalize(?) pre-release API/ABI versions

* Sat Jan 25 2014 Ian Pilcher <arequipeno@gmail.com> - 0.0.1
- Initial SPEC file
