Summary: cscope is an interactive, screen-oriented tool that allows the user to browse through C source files for specified elements of code.
Name: cscope
Version: 13.0
Release: 2
Copyright: BSD
Group: Development/Tools
Source: cscope-13.0.tar.gz
Buildroot: /tmp/%{name}-%{version}

%description
cscope is an interactive, screen-oriented tool that allows the user to browse through C source files for specified elements of code.

%prep
%setup

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/local/bin
mkdir -p $RPM_BUILD_ROOT/usr/local/man/man1

install -s -m 755 bin/cscope $RPM_BUILD_ROOT/usr/local/bin/cscope
install -m 755 doc/cscope.1 $RPM_BUILD_ROOT/usr/local/man/man1/cscope.1

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc TODO COPYING ChangeLog AUTHORS README NEWS

/usr/local/bin/cscope
/usr/local/man/man1/cscope.1

%changelog
* Sun Apr 16 2000 Petr Sorfa <petrs@sco.com>
- Initial Open Source release
- Ported to GNU environment
- Created rpm package
