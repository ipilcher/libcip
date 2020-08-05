%define	api_ver		0.1
%define so_ver		%{api_ver}.1
%define lib_ver		%{so_ver}.6

Name:		libcip
Summary:	C INI Parser
Version:	%{lib_ver}
Release:	1%{?dist}
License:	GPLv2
Source0:	https://github.com/ipilcher/%{name}/archive/%{version}.tar.gz#/%{name}-%{version}.tar.gz

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
gcc -g -Os -Wall -Wextra -shared -fPIC -fvisibility=hidden \
	-Wl,-soname,%{name}.so.%{so_ver} -o %{name}.so.%{version} *.c types/*.c

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_includedir}
cp %{name}.h %{buildroot}%{_includedir}
mkdir -p %{buildroot}%{_libdir}
cp %{name}.so.%{version} %{buildroot}%{_libdir}
ln -s %{name}.so.%{version} %{buildroot}%{_libdir}/%{name}.so

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%attr(0755,root,root) %{_libdir}/%{name}.so.%{version}
%{_libdir}/%{name}.so.%{so_ver}

%files devel
%attr(0644,root,root) %{_includedir}/%{name}.h
%{_libdir}/%{name}.so

%changelog
* Wed Aug  5 2020 Ian Pilcher <arequipeno@gmail.com> - 0.1.1.6-1
- Fix RPM build on EL 8

* Wed Oct  1 2014 Ian Pilcher <arequipeno@gmail.com> - 0.1.1.5-1
- Update SPEC for 0.1.1.5

* Fri Jan 31 2014 Ian Pilcher <arequipeno@gmail.com> - 0.1.1.4-2
- Use EL6-compatible ldconfig path in scriptlets
- Add release to changelog versions

* Fri Jan 31 2014 Ian Pilcher <arequipeno@gmail.com> - 0.1.1.4-1
- Version 0.1.1.4
- Fix versions in changelog

* Fri Jan 31 2014 Ian Pilcher <arequipeno@gmail.com> - 0.1.1.3-1
- Version 0.1.1.3

* Thu Jan 30 2014 Ian Pilcher <arequipeno@gmail.com> - 0.1.1.2-1
- Version 0.1.1.2
- Add dist tag

* Thu Jan 30 2014 Ian Pilcher <arequipeno@gmail.com> - 0.1.1.1-1
- Version 0.1.1.1

* Wed Jan 29 2014 Ian Pilcher <arequipeno@gmail.com> - 0.1.1.0-1
- Version 0.1.1.0

* Mon Jan 27 2014 Ian Pilcher <arequipeno@gmail.com> - 0.1.0.0-1
- Hide unnecessary symbols
- Optimize for size
- Rationalize(?) pre-release API/ABI versions

* Sat Jan 25 2014 Ian Pilcher <arequipeno@gmail.com> - 0.0.1
- Initial SPEC file
